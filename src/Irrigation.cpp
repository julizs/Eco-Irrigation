#include "Irrigation.h"

bool Irrigation::didEvaluate = false;
// int Irrigation::waterLimit2h = 500;
// int Irrigation::waterLimit24h = 5000;

/*
Do Query only once per timePeriod and not per SolenoidValve
Check Query in Console, "influx query"
*/
FluxQueryResult Irrigation::recentIrrigations(uint8_t timePeriod)
{
    char query[256] = "";
    snprintf(query, 256, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                         "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"pumpedWaterML\")"
                         "|> sum()",
             timePeriod);
    Serial.println(query);

    FluxQueryResult cursor = influxHelper.doQuery(query);
    return cursor;
}

/*
Only do Requests here that take long but require little disc space
Write InfluxDB Cursors to Vectors so waterLevel Evaluation on Button Press is fast
Use (huge) ArduinoJsonDocs only as local Vars that get destroyed if scope ends
*/
bool Irrigation::prepData()
{
    FluxQueryResult cursor2h = recentIrrigations(2);
    // FluxQueryResult cursor24h = Irrigation::recentIrrigations(24);
    while (!Utilities::writeVector(cursor2h, Utilities::ml2h))
        ;
    // while(!Irrigation::writeVector(cursor24h, Irrigation::vec24h));
    return true;
}

/*
Gets called by Irrigation Algo and at each Button Press
TODO check per Pump (multiple solenoidValves, Addition)
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve, uint16_t waterLimit)
{
    char message[64];

    for (auto const &s : Utilities::ml2h)
    {
        if (s.solenoidValve == solenoidValve)
        {
            snprintf(message, 64, "Solenoid Valve: %d, Water Amount: %d, Water Limit: %d", solenoidValve, s.waterAmount, waterLimit);
            Serial.println(message);

            if (s.waterAmount > waterLimit)
            {
                return false;
            }
        }
    }
    // No entry in Vec/InfluxDB or lower than Limit
    return true;
}

/* Irrigation Algorithm
  1.1 Skip if waterAmount released by this Solenoid over last 2h > Threshold
  1.2 (Ideal: Consider average soilMoisture of every Plant connected to this Solenoid over last 48h,
  but: 1 InfluxDB Request per Plant, slow)
  (Quick: Consider waterAmount released by this Solenoid over the last 48h)
  2. Consider Properties of each Plant in PlantGroup (Size, Dry Roots, ...)
  3. Decide Subjects (Plants -> same Solenoid/RelaisChannel -> Pump) and Milliliters
  3. Get necessary Info for Action (PumpModel, PumpTime, RelaisChannel, ...)
  4. Put ISubStateMachine Pointer on Stack, execute sequentially in ActionState
  5. Create Irrigation Datapoint for InfluxDB/MongoDB (even if Irrigation fails)
  (Plants: ..., Reason for Irrigation: ... , Irrigation Amount: ..., Reason for Failure: ...)
*/

/*
strcmp(plantName, name) == 0
*/
void Irrigation::decidePlants()
{
    char message[64];
    didEvaluate = false;
    uint8_t waterNeeds = 1, lightNeeds = 1, plantSize = 1; // e.g. plantSize 1 = 100-200mm

    // Do only 1 Request each
    DynamicJsonDocument plants(2048), plantNeeds(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/plants/needs", plantNeeds);
    // FluxQueryResult cursor2h = recentIrrigations(2);
    // vec2h = writeVector(cursor2h);

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];
        int solenoidValve = plants[i]["solenoidValve"].as<int>();

        if (!validSolenoid(solenoidValve, waterLimit24h)) // Efficient Check
        {
            continue; // skip Plant
        }

        for (int j = 0; j < plantNeeds.size(); j++)
        {
            const char *name = plantNeeds[j]["name"];
            if (plantName.equals(name))
            {
                waterNeeds = plantNeeds[j]["waterNeeds"];
                lightNeeds = plantNeeds[j]["lightNeeds"];
                plantSize = plantNeeds[j]["plantSize"];
            }

            snprintf(message, 64, "%s Needs: Water: %d, Light: %d, Plant Size: %d", name, waterNeeds, lightNeeds, plantSize);
            Serial.println(message);
        }
    }

    actions.add(&pump1);
    actions.add(&pump2);
    actions.add(&pump1);

    // Output: [["Thymian", 350], ["Aloe",280]]

    didEvaluate = true;
}

/*
Called by decideIrrigation and readCommands
Input: [["Thymian", 350], ["Aloe",280]]
Output: [[pump1, 0, 4.6],...]
*/
void Irrigation::writeInstructions(std::vector<Instruction> &instructions)
{
    DynamicJsonDocument plants(2048), pumps(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/pumps/json", pumps);

    for (auto &instr : instructions)
    {
        for (int i = 0; i < plants.size(); i++)
        {
            String plantName = plants[i]["name"];
            int solenoidValve = plants[i]["solenoidValve"].as<int>();

            if (!validSolenoid(solenoidValve, waterLimit24h)) // Efficient Check
            {
                continue; // goto next plant
            }

            if (plantName.equals(instr.plantName))
            {
                // Get SolenoidValve/relaisChannel
                instr.solenoidValve = solenoidValve;

                // Get pwmPin
                solenoidValve == (0 || 1) ? instr.pump = &pump1 : instr.pump = &pump2;

                // Get Pump Model
                for (int i = 0; i < pumps.size(); i++)
                {
                    JsonArray solenoidValves = pumps[i]["solenoidValve"];

                    for (int j = 0; j < solenoidValves.size(); j++)
                    {
                        // Serial.println(solenoidValves[j]);
                        if (solenoidValves[j] == solenoidValve)
                        {
                            // Add Pump Info
                            uint16_t flowRate = pumps[i]["flowRate"][0];
                            constrain(flowRate, 100, 500); // Avoid divide by 0
                            float pumpTime = (instr.waterAmount / (flowRate * 1000.0f)) * 3600;
                            instr.pumpTime = constrain(pumpTime, 0.0f, pumpTimeLimit);
                        }
                    }
                }
                // goto next instr (found match, stop iterating plants for this instr)
                continue;
            }
        }
    }
    reduceInstructions(instructions);
    printInstructions(instructions);  
}

void Irrigation::printInstructions(std::vector<Instruction> &instructions)
{
    char message[128];

    Serial.println("Irrigation Instructions: ");

    for (auto const &instr : instructions)
    {
        snprintf(message, 128, "Pump: %d, Pump Time: %0.2f, Solenoid Valve: %d, Water Amount: %d",
                 instr.pump, instr.pumpTime, instr.solenoidValve, instr.waterAmount);
        Serial.println(message);
    }
}

/*
For duplicate Instructions (Plants on same SolenoidValve): Take Min/Max/Avg waterAmount?
Vec must be sorted first (by SolenoidValve/relaisChannel) for unique to work
*/
void Irrigation::reduceInstructions(std::vector<Instruction> &instructions)
{ 
    char message[64];
    sortInstructions(instructions);
    auto it = std::unique(instructions.begin(), instructions.end());
    it == instructions.end() ? strcat(message,"No Dups") : strcat(message, "Removed Dups");
    Serial.println(message);
}

void Irrigation::sortInstructions(std::vector<Instruction> &instructions)
{
    std::sort(instructions.begin(), instructions.end(), compareBySolenoid);
}

bool Irrigation::compareBySolenoid(const Instruction &a, const Instruction &b)
{
    return a.solenoidValve > b.solenoidValve;
}

bool Irrigation::sendReport()
{
}

/* Get pwmPin
(Hardcoded, SolenoidValves/relaisChannels/pinNums 0, 1 are always
associated with pump1, just as pump_PWM_1 is always assoc. with pump1
User can only change pumpModel or enable/disable assoc. relaisChannels)
*/