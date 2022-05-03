#include "Irrigation.h"

bool Irrigation::didEvaluate = false;
std::vector<Instruction> Irrigation::irrInstructions;
std::vector<Instruction> Irrigation::pumpInstructions;

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

    FluxQueryResult cursor = InfluxHelper::doQuery(query);
    return cursor;
}

/*
Only do Requests here that take long but require little disc space
Write InfluxDB Cursors to Vectors so waterLevel Evaluation on Button Press is fast
Use (huge) ArduinoJsonDocs only as local Vars that get destroyed if scope ends
*/
bool Irrigation::requestData()
{
    FluxQueryResult cursor2h = recentIrrigations(2);
    // FluxQueryResult cursor24h = recentIrrigations(24);

    while (!Utilities::writeVector(cursor2h, Utilities::ml2h))
        ;
    // while (!Utilities::writeVector(cursor24h, Utilities::ml24h));
    Utilities::printMlPerSolenoid(Utilities::ml2h);
    // Utilities::printMlPerSolenoid(Utilities::ml24h);

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

/*
Call after processing manual User Irrigations
Check needs of each Plant in System
*/
void Irrigation::decidePlants()
{
    // instructions.clear();

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
            if (plantName.equals(name)) // strcmp(plantName, name) == 0
            {
                waterNeeds = plantNeeds[j]["waterNeeds"];
                lightNeeds = plantNeeds[j]["lightNeeds"];
                plantSize = plantNeeds[j]["plantSize"];
            }

            snprintf(message, 64, "%s Needs: Water: %d, Light: %d, Plant Size: %d", name, waterNeeds, lightNeeds, plantSize);
            Serial.println(message);
        }
    }

    // Output: [["Thymian", 350], ["Aloe",280]]
    didEvaluate = true;
}

/*
All actions (= planned instrs) get converted to instrs, if invalid (PumpModel not active, SolenoidValve not valid), set errorCode
Convert JsonObjects/JsonArrays to local Datastructures, Release Memory
irrActions and pumpActions are pushed on different Vecs
pumpInstructions need NO sorting, reducing etc.
*/
void Irrigation::createInstructions(JsonArray &actions, std::vector<Instruction> &instructions)
{
    if (!actions.isNull())
    {
        for (int i = 0; i < actions.size(); i++)
        {
            JsonObject action = actions.getElement(i);
            const char *subject = action["subject"];
            uint16_t allocatedWater = action["amount"];

            Instruction instr;
            snprintf(instr.reason, 32, subject);
            instr.allocatedWater = action["amount"];
            instructions.push_back(instr);
        }
    }
}

/*
Enrich Instructions with neccessary Infos for an Irrigation
Do Requests only once each
*/
void Irrigation::writeInstructions()
{
    DynamicJsonDocument plants(2048), pumps(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/pumps/json", pumps);

    for(auto &instr: irrInstructions)
    {
        uint8_t solenoidValve = getSolenoidByPlant(instr, plants);
        JsonObject pumpModel = findPumpModel(pumps, solenoidValve, "");
        pickSolenoidValve(instr, pumpModel);
        setPumpDetails(instr, pumpModel);
    }

    for(auto &instr: pumpInstructions)
    {
        JsonObject pumpModel = findPumpModel(pumps, -1, instr.reason);
        // JsonArray solenoidValves = pumpModel["solenoidValves"];
        pickSolenoidValve(instr, pumpModel);
        setPumpDetails(instr, pumpModel);
    }

    reduceInstructions(irrInstructions);
}

/*
Create Instruction Vec and fill with necessary Infos
Called by decideIrrigation and readCommands
Input: [["Thymian", 350], ["Aloe",280]]  (both on same solenoidValve)
Output: [[pump1, 0, 4.6],...]
*/
uint8_t Irrigation::getSolenoidByPlant(Instruction &instr, DynamicJsonDocument &plants)
{
    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];
        uint8_t solenoidValve = plants[i]["solenoidValve"].as<int>();

        if (plantName.equals(instr.reason))
        {
            return solenoidValve;
        }
    }
    return -1;
}

/*
Check if soleonoidValve or any of the solenoidValves (= pumpModel) are valid
Pick the first valid one, set pwmPin accordingly
Set Error Codes
instr.pump = (solenoidValve == (0 || 1)) ? &pump1 : &pump2; // pump1: 0 || 1, pump2: 2
*/
bool Irrigation::pickSolenoidValve(Instruction &instr, JsonObject pumpModel) // uint8_t solenoidValves[]
{
    if(!pumpModel.isNull())
    {
        JsonArray solenoidValves = pumpModel["solenoidValve"];
        if(!solenoidValves.isNull())
        {
            for(auto sol : solenoidValves)
            {
                if(validSolenoid(sol, waterLimit24h))
                {
                    instr.solenoidValve = sol;
                    instr.pump = (instr.solenoidValve == 0) ? &pump1 : &pump2;
                    instr.errorCode = 0;
                    return true;
                }
                // No valid solenoidValve for found pumpModel
                instr.errorCode = 1;
                return false;
            }
        }
    }
    // No active pumpModel for given solenoidValve
    instr.errorCode = 2;
    return false;

    /*
    int size_t = sizeof(solenoidValves) / sizeof(solenoidValves[0]);

    for(int i = 0; i < size_t; i++)
    {
        if(validSolenoid(solenoidValves[i], waterLimit24h))
        {
            instr.solenoidValve = solenoidValves[i];
            instr.pump = (instr.solenoidValve == 0) ? &pump1 : &pump2; // pwmPin
            instr.errorCode = 0;
            return true;
        }
    }
    instr.errorCode = 1;
    return false;
    */
}

/*
Find pumpModel by solenoid or by name and make sure they are active
*/
JsonObject Irrigation::findPumpModel(DynamicJsonDocument &pumps, uint8_t solenoidValve, const char pumpName[])
{
    for (int i = 0; i < pumps.size(); i++)
    {
        JsonArray solenoidValves = pumps[i]["solenoidValve"];
        String modelName = pumps[i]["name"];

        if (sizeof(pumpName) == 0)
        {
            for (int j = 0; j < solenoidValves.size(); j++)
            {
                if (solenoidValves[j] == solenoidValve)
                {
                    return pumps[i];
                }
            }
        }
        else
        {
            if(modelName.equals(pumpName))
            {
                if(!solenoidValves.isNull())
                    return pumps[i];
            }
        }
    }   
}

void Irrigation::setPumpDetails(Instruction &instr, JsonObject pumpModel)
{
    snprintf(instr.pumpModel, 32, pumpModel["name"]);
    uint16_t flowRate = pumpModel["flowRate"][0];
    constrain(flowRate, 100, 500); // Avoid divide by 0

    float pumpTime = (instr.allocatedWater / (flowRate * 1000.0f)) * 3600;
    instr.pumpTime = constrain(pumpTime, 0.0f, pumpTimeLimit);
}

void Irrigation::printInstructions()
{
    char message[128];

    Serial.println("Irrigation Instructions: ");
    for (auto const &instr : irrInstructions)
    {
        snprintf(message, 128, "Reason: %s, Pump: %d, Pump Time: %0.2fs, Solenoid Valve: %d, Allocated Water: %dml, ErrorCode: %d",
                 instr.reason, instr.pumpModel, instr.pumpTime, instr.solenoidValve, instr.allocatedWater, instr.errorCode);
        Serial.println(message); // instr.pump->pwmPin
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
    it == instructions.end() ? strcat(message, "No Dups") : strcat(message, "Removed Dups");
    Serial.println(message);
}

void Irrigation::sortInstructions(std::vector<Instruction> &instructions)
{
    std::sort(instructions.begin(), instructions.end(), compareBySolenoid);
}

/*
Also sort by: pumpTime ?
*/
bool Irrigation::compareBySolenoid(const Instruction &a, const Instruction &b)
{
    return a.solenoidValve > b.solenoidValve;
}

/* InfluxDB:
Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
Tags are indexed (unlike Fields), faster Queries
-> e.g. which Irrigations failed (errorCode != 0) ?
*/
bool Irrigation::reportInstructions()
{
    StaticJsonDocument<1024> reports;
    String output;

    for (auto const &instr : irrInstructions)
    {
        // Write to InfluxDB
        Point p2("Irrigations");
        // p2.clearTags();
        // p2.clearFields();
        char solenoidValve[4], errorCode[4];
        itoa(instr.solenoidValve, solenoidValve, 10);
        itoa(instr.errorCode, errorCode, 10);

        p2.addTag("reason", instr.reason);
        p2.addTag("pump", instr.pumpModel);
        p2.addTag("solenoidValve", solenoidValve);
        p2.addTag("errorCode", errorCode);
        p2.addField("waterAmount", instr.allocatedWater);
        p2.addField("pumpTime", instr.pumpTime);
        InfluxHelper::writeDataPoint(p2);

        // Write to MongoDB
        // https://arduinojson.org/v6/api/json/serializejson/
        JsonObject report;
        report["reason"] = instr.reason;
        report["pump"] = instr.pumpModel;
        report["solenoidValve"] = instr.solenoidValve;
        report["errorCode"] = instr.errorCode;
        report["allocatedWater"] = instr.allocatedWater; // Planned
        // report["distributedWater"] = instr.allocatedWater; // Actual
        // report["timeStamp"] = ...
        report["pumpTime"] = instr.pumpTime;
        reports.add(report);
    }

    serializeJson(reports, output);
    Services::doPostRequest("/settings/report", output);
}

/* Get pwmPin
(Hardcoded, SolenoidValves/relaisChannels/pinNums 0, 1 are always
associated with pump1, just as pump_PWM_1 is always assoc. with pump1
User can only change pumpModel or enable/disable assoc. relaisChannels)
*/

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