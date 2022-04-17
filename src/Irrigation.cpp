#include "Irrigation.h"

bool Irrigation::didEvaluate = false;
int Irrigation::waterLimit2h = 500;
int Irrigation::waterLimit24h = 5000;
std::vector<SolenoidMl> Irrigation::vec2h, vec24h;

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

bool Irrigation::writeVector(FluxQueryResult &cursor, std::vector<SolenoidMl> &vec)
{
    // std::vector<SolenoidMl> vec;
    while (cursor.next())
    {
        uint8_t solenoidValve = cursor.getValueByName("solenoidValve").getLong();
        uint16_t waterAmount = cursor.getValueByName("_value").getLong();
        bool exists = false;

        for (auto &s : vec)
        {
            if (s.solenoidValve == solenoidValve)
            {
                s.waterAmountMl += waterAmount;
                exists = true;
            }
        }
        if (!exists)
        {
            SolenoidMl sol;
            sol.solenoidValve = solenoidValve;
            sol.waterAmountMl = waterAmount;
            vec.push_back(sol);
        }

        if (cursor.getError() != "")
        {
            Serial.println("Vector write Error: ");
            Serial.println(cursor.getError());
        }
    }
    Serial.println("Vector write Success.");
    return true;
}

/*
Gets called by Irrigation Algo and at each Button Press
TODO check per Pump (multiple solenoidValves, Addition)
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve, uint16_t waterLimit)
{
    char message[64];

    for (auto const &s : vec2h)
    {
        if (s.solenoidValve == solenoidValve)
        {
            snprintf(message, 64, "Solenoid: %d, Water: %d, Water Limit: %d", solenoidValve, s.waterAmountMl, waterLimit);
            Serial.println(message);

            if (s.waterAmountMl > waterLimit)
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
void Irrigation::writeInstruction(std::vector<instruction> &instruction)
{
    std::vector<SolenoidMl> sols;
    DynamicJsonDocument plants(2048), pumps(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/pumps", pumps);

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];
        int solenoidValve = plants[i]["solenoidValve"].as<int>();

        if (!validSolenoid(solenoidValve, waterLimit24h)) // Efficient Check
        {
            continue; // skip Plant
        }

        for (auto &s : instruction)
        {
            if (plantName.equals(s.plantName))
            {
                // Add Solenoid Info
                s.solenoidValve = solenoidValve;

                // Select correct Pump
                for (int i = 0; i < pumps.size(); i++)
                {
                    JsonArray solenoidValves = pumps[i]["solenoidValve"];
                    
                    for(int j = 0; j < solenoidValves.size(); j++)
                    {
                        if(solenoidValves[j] == solenoidValve)
                        {
                            // Add Pump Info
                            uint16_t flowRate = pumps[i]["flowRate"][0];
                            s.pumpTime = s.waterAmountMl/(flowRate*1000.0f);
                            // s.pump = pump1;
                        }
                    }
                }
            }
        }
    }

    // Plants -> relaisChannel/Solenoids
    // Solenoids -> Pump
    /*
    int litersPerHour = pump["flowRate"][1];
    int pwmChannel = pump["pwmChannel"];

    // avoid DivideByZero if Attr not available
    if (litersPerHour == 0)
    {
        litersPerHour = 300;
    }

    float pumpTime = (irrigationAmount / (litersPerHour * 1000.0f)) * 3600;

    // Serial.print("Irrigate: "); Serial.println(plantGroup);
    Serial.println(pwmChannel);
    Serial.println(pumpTime);
    */
}

/*
char query[32] = "/";
strcat(query, plantName);
strcat(query, "/needs");
DynamicJsonDocument needs = Services::doJSONGetRequest(query);
int currentSize = needs[0]["plantSize"];
Serial.println(currentSize);
*/