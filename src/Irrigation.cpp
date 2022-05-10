#include "Irrigation.h"

std::vector<Instruction> Irrigation::instructions;
std::vector<WaterPerSolenoid> Irrigation::waterPerSol;

/*
Query is done only once for ALL Solenoids in System, per timePeriod
Check Query in Powershell/Console, "influx query"
*/
FluxQueryResult Irrigation::recentIrrigations(uint8_t timePeriod)
{
    char query[256] = "";
    snprintf(query, 256, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                         "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"allocatedWater\")"
                         "|> sum()",
             timePeriod);

    FluxQueryResult cursor = InfluxHelper::doQuery(query);
    // Serial.println(query);
    // Utilities::printCursor(cursor);

    return cursor;
}

/*
Get all recent Irrigations (in ml) per Solenoid
Write InfluxDB Cursors to Vecs so waterLimit Evaluation is fast
*/
bool Irrigation::updateData()
{
    uint8_t timePeriod = 24;
    FluxQueryResult cursor = recentIrrigations(timePeriod);

    while (!Utilities::cursorToVec(cursor, waterPerSol, timePeriod))
        ;

    timePeriod = 48;
    cursor = recentIrrigations(timePeriod);

    while (!Utilities::cursorToVec(cursor, waterPerSol, timePeriod))
        ;

    // Utilities::printVector(waterPerSol);

    return true;
}

/*
Gets called by Irrigation Algo and at each Button Press
TODO check per Pump (multiple solenoidValves, Addition)
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve, uint16_t waterLimit, u16_t allocatedWater, uint8_t timePeriod)
{
    char message[128];

    for (auto const &s : waterPerSol)
    {
        if (s.solenoidValve == solenoidValve && s.timePeriod == timePeriod)
        {
            snprintf(message, 128, "Solenoid Valve: %d, Water Amount: %dml, Water Limit: %dml, Time Period: %dh, Valid: %s", 
            solenoidValve, s.waterAmount, waterLimit, timePeriod, ((s.waterAmount + allocatedWater) < waterLimit ? "true" : "false"));
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
bool Irrigation::decidePlants()
{
    char message[64];
    uint8_t waterNeeds = 1, lightNeeds = 1, plantSize = 1; // e.g. plantSize 1 = 100-200mm

    // Do only 1 Request each
    DynamicJsonDocument plants(2048), plantNeeds(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/plants/needs", plantNeeds);

    Serial.println("Plant Needs: ");

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];
        int solenoidValve = plants[i]["solenoidValve"].as<int>();

        /*
        At this point unclear if validSolenoid, since allocatedWater not known
        if (!validSolenoid(solenoidValve, waterLimit24h, 24)) // Efficient Check
        {
            continue; // skip Plant
        }
        */

        for (int j = 0; j < plantNeeds.size(); j++)
        {
            const char *name = plantNeeds[j]["name"];
            if (plantName.equals(name)) // strcmp(plantName, name) == 0
            {
                waterNeeds = plantNeeds[j]["waterNeeds"];
                lightNeeds = plantNeeds[j]["lightNeeds"];
                plantSize = plantNeeds[j]["plantSize"];
                snprintf(message, 64, "%s: Water: %d, Light: %d, Plant Size: %d", name, waterNeeds, lightNeeds, plantSize);
                Serial.println(message);
            }
        }
    }

    // Output: createInstructions, same as incoming User Actions
    // [["Thymian", 350], ["Aloe",280]]
    // writeInstructions();
    return true;
}

/*
All actions (= planned instrs) get converted to instrs, if invalid (PumpModel not active, SolenoidValve not valid), set errorCode
Convert JsonObjects/JsonArrays to local Datastructures, Release Memory
irrActions and pumpActions are pushed on different Vecs
pumpInstructions need NO sorting, reducing etc.
*/
void Irrigation::createInstructions(JsonArray &actions, std::vector<Instruction> &instructions)
{
    JsonObject action;

    if (!actions.isNull())
    {
        for (int i = 0; i < actions.size(); i++)
        {
            action = actions.getElement(i);
            const char *subject = action["subject"];
            uint16_t allocatedWater = action["amount"];

            Instruction instr;
            snprintf(instr.reason, 32, subject);
            instr.allocatedWater = allocatedWater;
            instructions.push_back(instr);
        }
    }
}

/*
Write all neccessary Infos for an Irrigation into Instr
Do Requests only once
Manual Irrigations are reduced too
*/
void Irrigation::writeInstructions()
{
    DynamicJsonDocument plants(2048), pumps(1024);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/pumps/json", pumps);

    for (auto &instr : instructions)
    {
        JsonObject pumpModel = pumpModelByName(pumps, instr.reason);
        if (!pumpModel.isNull()) // instr.reason: a pump
        {
            solenoidByPump(instr, pumpModel);
            setPumpTime(instr, pumpModel);
        }
        else // instr.reason: a plant
        {
            solenoidByPlant(instr, plants);
            JsonObject pumpModel = pumpModelBySolenoid(pumps, instr.solenoidValve);
            setPumpTime(instr, pumpModel);
        }
    }

    reduceInstructions(instructions);
    printInstructions(instructions);
}

/*
Create Instruction Vec and fill with necessary Infos
Called by decideIrrigation and readCommands
Input: [["Thymian", 350], ["Aloe",280]]  (both on same solenoidValve)
Output: [[pump1, 0, 4.6],...]
break: found matching solenoid for plant (valid or invalid), break for loop
*/
int8_t Irrigation::solenoidByPlant(Instruction &instr, DynamicJsonDocument &plants)
{
    // 1: found no matching plant for instr.reason
    int8_t solenoid = -1, errorCode = 1;

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];

        if (plantName.equals(instr.reason))
        {
            if (validSolenoid(solenoid, waterLimit24h, instr.allocatedWater, 24))
            {
                solenoid = plants[i]["solenoidValve"].as<int>();
                errorCode = 0;
            }
            else
            {
                errorCode = 2;
            }
            break;
        }
    }

    instr.errorCode = errorCode;
    instr.solenoidValve = solenoid;

    return solenoid;
}

/*
Check if any of the solenoidValve of pumpModel (= pumpModel) are valid
Pick the first valid one, set pwmPin accordingly
Set Error Codes
break: found the first valid solenoid for model, break for loop
*/
int8_t Irrigation::solenoidByPump(Instruction &instr, JsonObject &pumpModel)
{
    int8_t solenoid = -1, errorCode = 1; // pumpModel has no Solenoids at all (= model not active)

    if (!pumpModel.isNull())
    {
        JsonArray solenoidValves = pumpModel["solenoidValve"];
        if (!solenoidValves.isNull())
        {
            for (auto sol : solenoidValves)
            {
                if (validSolenoid(sol, waterLimit24h, instr.allocatedWater, 24))
                {
                    solenoid = sol;
                    errorCode = 0;
                    break;
                }
                else
                {
                    // Solenoid not valid, keep Searching
                    errorCode = 2;
                }
            }
        }
    }

    instr.errorCode = errorCode;
    instr.solenoidValve = solenoid;
    return solenoid;
}

/*
Find pumpModel by solenoid or name
Found pumpModel is active in System (has solenoids)
*/
JsonObject Irrigation::pumpModelBySolenoid(DynamicJsonDocument &pumps, uint8_t solenoidValve)
{
    JsonObject pumpModel;

    for (int i = 0; i < pumps.size(); i++)
    {
        JsonArray solenoidValves = pumps[i]["solenoidValve"];
        String modelName = pumps[i]["name"].as<String>();

        for (int j = 0; j < solenoidValves.size(); j++)
        {
            if (solenoidValves[j] == solenoidValve)
            {
                // pumpModel is active
                pumpModel = pumps[i].as<JsonObject>();
                break;
            }
        }
    }
    return pumpModel;
}

JsonObject Irrigation::pumpModelByName(DynamicJsonDocument &pumps, const char pumpName[])
{
    JsonObject pumpModel;

    for (int i = 0; i < pumps.size(); i++)
    {
        JsonArray solenoidValves = pumps[i]["solenoidValve"];
        String modelName = pumps[i]["name"].as<String>();

        if (modelName.equals(pumpName))
        {
            // if (!solenoidValves.isNull()) // pumpModel is active
            pumpModel = pumps[i].as<JsonObject>();
            break; // Stop searching
        }
    }
    return pumpModel;
}

/*
SolenoidValves/relaisChannels/pinNums 0, 1 are assoc. with pump1,
and pump_PWM_1 is assoc. with pump1
(depends on Hw Config of Watertank/Pump/Solenoids : n: n: n)
User can only change pumpModel or enable/disable assoc. relaisChannels on the Fly)
instr.pump = (solenoidValve == (0 || 1)) ? &pump1 : &pump2; // pump1: 0 || 1, pump2: 2
*/
void Irrigation::setPumpTime(Instruction &instr, JsonObject &pumpModel)
{
    instr.pump = (instr.solenoidValve == 0) ? &pump1 : &pump2; // Set pwmPin

    snprintf(instr.pumpModel, 32, pumpModel["name"]);
    uint16_t flowRate = pumpModel["flowRate"][0];
    constrain(flowRate, 100, 500); // Avoid divide by 0

    float pumpTime = (instr.allocatedWater / (flowRate * 1000.0f)) * 3600;
    instr.pumpTime = constrain(pumpTime, 0.0f, pumpTimeLimit);
}

/*
Print Vec to Serial
*/
void Irrigation::printInstructions(std::vector<Instruction> &instructions)
{
    char message[128];

    Serial.print("Irrigation Instructions: ");
    if (instructions.size() == 0)
        Serial.println("None.");
    else
    {
        Serial.println();
        for (auto const &instr : instructions)
        {
            snprintf(message, 128, "Reason: %s, Pump Model: %s, Pump Time: %0.2fs, Solenoid Valve: %d, Allocated Water: %dml, ErrorCode: %d",
                     instr.reason, instr.pumpModel, instr.pumpTime, instr.solenoidValve, instr.allocatedWater, instr.errorCode);
            Serial.println(message); // instr.pump->pwmPin
        }
    }
}

/*
For duplicate Instructions (Plants on same SolenoidValve): Take Min/Max/Avg waterAmount?
Vec must be sorted first (by SolenoidValve/relaisChannel) for unique to work
strcat is unsafe, stack smashing crash if char[] too small
*/
void Irrigation::reduceInstructions(std::vector<Instruction> &instructions)
{
    char message[32] = "";
    sortInstructions(instructions);
    auto it = std::unique(instructions.begin(), instructions.end());
    it == instructions.end() ? strcat(message, "No Duplicate Instructions") : strcat(message, "Removed Duplicate Instructions");
    Serial.println(message);
}

/*
Also sort by: pumpTime ?
*/
void Irrigation::sortInstructions(std::vector<Instruction> &instructions)
{
    std::sort(instructions.begin(), instructions.end(), compareBySolenoid);
}

bool Irrigation::compareBySolenoid(const Instruction &a, const Instruction &b)
{
    return a.solenoidValve > b.solenoidValve;
}

/* Report to InfluxDB:
Tags are indexed (unlike Fields), faster Queries
-> Use Tags instead of Fields for often queried Attr
Tags can only be Strings
e.g. which Irrigations failed (errorCode != 0) ?
Check (as Table): influx query
from(bucket: "messdaten")
|> range(start: -24h)
|> filter(fn: (r) => r._measurement == "Irrigations")
*/
bool Irrigation::reportInstructions(std::vector<Instruction> &instructions)
{
    for (auto const &instr : instructions)
    {
        Point p2("Irrigations");
        // p2.clearTags(); p2.clearFields();
        char solenoidValve[4], errorCode[4];
        itoa(instr.solenoidValve, solenoidValve, 10);
        itoa(instr.errorCode, errorCode, 10);

        p2.addTag("reason", instr.reason);
        p2.addTag("pump", instr.pumpModel);
        p2.addTag("solenoidValve", solenoidValve);
        p2.addField("allocatedWater", instr.allocatedWater);
        // p2.addField("pumpTime", instr.pumpTime);
        p2.addTag("errorCode", errorCode);
        InfluxHelper::writeDataPoint(p2); // Write to Buffer
    }
    return true;
}

/*
Call after reportInstructions
*/
void Irrigation::clearInstructions()
{
    instructions.clear();
}

/*
Report to MongoDB
https://arduinojson.org/v6/api/json/serializejson/

bool Irrigation::reportToMongo(std::vector<Instruction> &instructions)
{
    StaticJsonDocument<2048> reports;
    String output;

    for (auto const &instr : instructions)
    {
        JsonObject report;
        report["reason"] = instr.reason;
        report["pump"] = instr.pumpModel;
        report["solenoidValve"] = instr.solenoidValve;
        report["allocatedWater"] = instr.allocatedWater; // Planned
        // report["distributedWater"] = instr.allocatedWater; // Actual
        // report["timeStamp"] = ...
        report["pumpTime"] = instr.pumpTime;
        report["errorCode"] = instr.errorCode;
        reports.add(report);
    }

    // output = "{}";
    serializeJson(reports, output);
    Services::doPostRequest("/settings/report", output);
    return true;
}
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