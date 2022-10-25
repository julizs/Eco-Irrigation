#include "Irrigation.h"

std::vector<Instruction> Irrigation::instructions;
std::vector<WaterPerSolenoid> Irrigation::recentIrrigations; // waterDistribution
// std::string Irrigation::errors[] = {"Test"};
String Irrigation::errors[] = {"None", "Plant not found", "Solenoid invalid (Not existing)", "Solenoid invalid (Waterlimit)",
                             "Pump has no Solenoids assigned.", "Pump has no valid Solenoids (Waterlimit)",
                             "Pump currently not active in System"};

/*
Check recent waterAmounts for ALL Solenoids per timePeriod
(Check with Powershell/Console or make new Panel in Grafana):

influx query, Enter

from(bucket: "messdaten")
|> range(start: -24h)
|> filter(fn: (r) => r._measurement == "Irrigations" and r._field == "distributedWater" and r["errorCode"] == "0")
|> sum()

from(bucket: "messdaten")
|> range(start: -24h)
|> filter(fn: (r) => r._measurement == "Irrigations" and r._field == "distributedWater" and r["errorCode"] == "0")
|> group(columns: ["solenoidValve"])
|> sum()

Enter, Strg + d
*/
FluxQueryResult Irrigation::querySolenoids(uint8_t timePeriod)
{
    char query[512] = "";
    snprintf(query, 512, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                         "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"distributedWater\" and r[\"errorCode\"] == \"0\")"
                         "|> group(columns: [\"solenoidValve\"])"
                         "|> sum()",
             timePeriod);

    FluxQueryResult cursor = InfluxHelper::doQuery(query);
    // Serial.println(query);
    // Utilities::printCursor(cursor);

    return cursor;
}

/*
https://www.sqlpac.com/en/documents/influxdb-moving-from-influxql-language-to-flux-language.html
https://docs.influxdata.com/influxdb/cloud/query-data/common-queries/iot-common-queries/
https://docs.influxdata.com/influxdb/cloud/query-data/flux/monitor-states/
How many Minutes were roots wet today
How many Sun Hours did Plant receive today
-> Influences Plant Happiness and Irrigation
*/
uint8_t Irrigation::queryMoisture(uint8_t threshold, uint8_t timePeriod)
{
    char query[256] = "";
    snprintf(query, 256, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                         "|> filter(fn: (r) => r._measurement == \"Plant Data\" and r._field == \"Soil Moisture\")"
                         "|> sum()",
             timePeriod);

    FluxQueryResult cursor = InfluxHelper::doQuery(query);
    

}

/*
Get all recent Irrigations (in ml) per Solenoid per time period (2 hours, 1 day, 1 week)
to compare to waterLimit
Goal: Only 1 db request for all validSolenoid() checks, less delay writing Instructions
Write InfluxDB Cursors to vector container
*/
bool Irrigation::getRecentIrrigations()
{
    recentIrrigations.clear();

    uint8_t timePeriods[] = {2, 24, 168};
    for (int i = 0; i < 3; i++)
    {
        FluxQueryResult cursor = querySolenoids(timePeriods[i]);

        // If any query fails, retry in next selfTransition
        if(cursor.getError() != "")
            return false;

        while (!Utilities::cursorToVec(cursor, recentIrrigations, timePeriods[i]))
            ;
    }

    Utilities::printSolenoids(recentIrrigations);

    return true;
}

/*
Needed for Irrigation Algo
*/
uint16_t Irrigation::waterPerSolenoid(uint8_t solenoidValve, uint8_t timePeriod)
{
    uint16_t waterAmount = 0;

    for (auto const &sol : recentIrrigations)
    {
        if (sol.solenoidValve == solenoidValve && sol.timePeriod == timePeriod)
        {
            waterAmount = sol.waterAmount;
        }
    }
    return waterAmount;
}

/*
Needed for Evaluation (Manual Irr and Irr Algo)
TODO valid Pump (has min. 1 valid Solenoid?)
Outcome = solenoid still within waterLimit after Irrigation?
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve, uint16_t waterLimit, u16_t allocatedWater, uint8_t timePeriod)
{
    char message[128];

    uint16_t distributedWater = waterPerSolenoid(solenoidValve, timePeriod);
    uint16_t outcome = distributedWater + allocatedWater;

    /*
    snprintf(message, 128, "Solenoid Valve: %d, Water Amount: %dml, Time Period: %dh, Valid: %s",
    solenoidValve, waterAmount, timePeriod, outcome < waterLimit ? "true" : "false"); Serial.println(message);
    */

    return outcome < waterLimit;
}

/*
Part of Evaluate Plants / Happiness?
Call after processing manual User Irrigations
Check needs of each Plant in System
e.g. plantSize 1 = 100-200mm, waterNeeds 1 = 500ml per 2 days

EVALUATE State checks overall Plant Wellbeing first and displays Happiness
this Func only processes all Water related Values from that Query, decides Irrigations
*/
bool Irrigation::evaluatePlants()
{
    char message[128];
    uint8_t waterNeeds = 5, lightNeeds = 5, tempNeeds = 5, humidityNeeds = 0; // Standard Values (1-10)
    bool letDry, fullSun;
    int k = 0;

    DynamicJsonDocument plants(2048), plantNeeds(2048);
    Services::doJSONGetRequest("/plants/sensors", plants);
    Services::doJSONGetRequest("/plants/needs", plantNeeds);
    // Services::doJSONGetRequest("/plants/happiness", plantHappiness);

    Serial.println("Plant Needs: ");

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["scientificName"];
        int solenoidValve = plants[i]["solenoidValve"].as<int>();

        for (int j = 0; j < plantNeeds.size(); j++)
        {
            const char *name = plantNeeds[j]["scientificName"];
         
            if (plantName.equals(name))
            {
                waterNeeds = plantNeeds[j]["waterNeeds"];
                lightNeeds = plantNeeds[j]["lightNeeds"];
                tempNeeds = plantNeeds[j]["tempNeeds"];
                tempNeeds = plantNeeds[j]["humidityNeeds"];
                snprintf(message, 128, "%s: Water: %d, Light: %d, Temperature: %d, Humidity: %d", name, waterNeeds, lightNeeds, tempNeeds, humidityNeeds);
                Serial.println(message);
                k++;
            }
        }
    }

    if(k == 0)
        Serial.println("No Entries for given Plants found.");

    // Output: createInstructions, same as incoming User Actions
    // [["Thymian", 350], ["Aloe",280]]
    // writeInstructions();
    return true;
}

/*
bool Irrigation::decidePlant()
{

}
*/

/*
Create instructions from user actions (= "manual" instr)
decidePlants also creates instructions (= "autonomous" instr)
Create instrs even if invalid (e.g. PumpModel not active, SolenoidValve not valid), 
set errorCode, still send to mongoDB or InfluxDB
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
Enrich Instr with all neccessary Infos for Irrigation
instr.reason
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
        // User clicked on Pump!, e.g. instr.reason: "QR50E"
        if (!pumpModel.isNull()) 
        {
            solenoidByPump(instr, pumpModel); // Find any valid Solenoid connected to this Pump
            setPumpingParameters(instr, pumpModel);
        }
        else // User clicked on Irrigate! e.g. instr.reason: "Basilikum"
        {
            solenoidByPlant(instr, plants); // Check if this Plant is connected to a Sol. and if Sol. is valid
           
            JsonObject pumpModel = pumpModelBySolenoid(pumps, instr.solenoidValve);
            setPumpingParameters(instr, pumpModel);
        }
    }
 
    // Not necessary, as Water gets split by Dispenser (Testing needed)
    // reduceInstructions(instructions);
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
    // errorCode 1: found no matching plant for instr.reason
    int8_t solenoid = -1, errorCode = 1;
    // Serial.println("Recently Distributed Water:");

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];

        if (plantName.equals(instr.reason))
        {
            solenoid = plants[i]["solenoidValve"].as<int>();
 
            if(solenoid > 0 && solenoid <= 2)
            {
                if (validSolenoid(solenoid, waterLimit24h, instr.allocatedWater, 24))
                {          
                    errorCode = 0;
                }
                else
                {
                    // Solenoid already distributed too much water in recent 24h
                    errorCode = 3;
                }
                break;
            }
            else
            {
                // Solenoid currently does not exist in System (null = Plant not connected) 
                // or User set invalid Solenoid in Interface (e.g. 12)
                errorCode = 2;
            }   
        }
    }

    instr.errorCode = errorCode;
    instr.solenoidValve = solenoid;

    return solenoid;
}

/*
aka validPump()
Checks if pumpModel has any associated solenoids at all
Checks if any of the solenoids are valid
Pick the first valid one, set pwmPin accordingly
Set Error Codes otherwise
break: found the first valid solenoid for model, break for loop
Gets called by manual Irrigations (Pumpcheck), so validify with 2h waterLimit
*/
int8_t Irrigation::solenoidByPump(Instruction &instr, JsonObject &pumpModel)
{
    int8_t solenoid = -1, errorCode = 4; // pumpModel has no Solenoids assigned (= currently not active)

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
                    // Solenoid reached Waterlimit, check next
                    // Or: All assigned Solenoids reached Waterlimit
                    errorCode = 5;
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

/*
User initiates Pump Process of QR50E in Grafana
Name as String Param
*/
JsonObject Irrigation::pumpModelByName(DynamicJsonDocument &pumps, const char pumpName[])
{
    JsonObject pumpModel;

    for (int i = 0; i < pumps.size(); i++)
    {
        // JsonArray solenoidValves = pumps[i]["solenoidValve"];
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
pumpModel has pwmChannel field (mongoDB)
(Gives Info if pumpModel is currently connected and at which pin)
Using Pump without solenoid Valves should be possible
1-n Pumps per Cistern should be possible
Connect/Disconnect solenoid Valve to Pump should be possible
Connect/Disconnect Pump in System = Change pwmChannel field in mongoDB
pwmChannel/Pin always associated with same pumpObj (no changes necessary)
*/
void Irrigation::setPumpingParameters(Instruction &instr, JsonObject &pumpModel)
{
    if(pumpModel.isNull())
    {
        snprintf(instr.pumpModel, 32, "Null");
        return;
    }

    // Adress correct pumpObj / pwmChannel
    int pwmChannel = pumpModel["pwmChannel"];
    if(pwmChannel == 1)
    {
        instr.pump = &pump2; // toF 0x52 not connected, dont use pump1
    }     
    else if(pwmChannel == 2)
    {
        instr.pump = &pump1;
    }    
    else
    {
        // Pump currently not active in System
        instr.errorCode = 6;
    }

    snprintf(instr.pumpModel, 32, pumpModel["name"]);

    uint16_t flowRate = pumpModel["flowRate"][0];
    constrain(flowRate, 100, 500); // Avoid divide by 0

    // flowRate in L/h, allocatedWater in ml, pumpTime in sec
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
            snprintf(message, 128, "Subject: %s, Pump Model: %s, Pump Time: %0.2fs, Solenoid Valve: %d, Allocated Water: %dml, ErrorCode: %d",
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
Also implement sort by: pumpTime ?
*/
void Irrigation::sortInstructions(std::vector<Instruction> &instructions)
{
    std::sort(instructions.begin(), instructions.end(), compareBySolenoid);
}

bool Irrigation::compareBySolenoid(const Instruction &i1, const Instruction &i2)
{
    return i1.solenoidValve > i2.solenoidValve;
}

/* Report to InfluxDB:
Do NOT write Points at the same Time / with same Timestamp since 
Grafana Points in Graphs will be overlapped / hard to read

Tags are indexed (unlike Fields), faster Queries
-> Use Tags instead of Fields for often queried Attr
Tags can only be Strings
e.g. which Irrigations failed (errorCode != 0) ?
Check (as Table) in Powershell: 
influx query

Strg+c, Rechtsklick zum Einfügen:

from(bucket: "messdaten")
|> range(start: -5m)
|> filter(fn: (r) => r._measurement == "Irrigations")

Enter, Strg+d zum Ausführen
*/
/*
bool Irrigation::reportInstructions(std::vector<Instruction> &instructions)
{

    for (auto const &instr : instructions)
    {
        // Point p2("Irrigations");
        p2.clearTags(); p2.clearFields();

        // Tags have to be Strings
        char solenoidValve[4], errorCode[4];
        itoa(instr.solenoidValve, solenoidValve, 10);
        itoa(instr.errorCode, errorCode, 10);

        p2.addTag("reason", instr.reason);
        p2.addTag("pump", instr.pumpModel);
        p2.addTag("solenoidValve", solenoidValve);
        p2.addField("allocatedWater", instr.allocatedWater);
        p2.addField("distributedWater", instr.distributedWater);
        // p2.addField("pumpTime", instr.pumpTime);
        p2.addTag("errorCode", errorCode);
        InfluxHelper::writeDataPoint(p2); // Write to Buffer
    }

    clearInstructions();

    return true;
}
*/

bool Irrigation::reportInstruction(Instruction &instr)
{
    p2.clearTags(); p2.clearFields();

    // Tags have to be Strings
    char solenoidValve[4], errorCode[4];
    itoa(instr.solenoidValve, solenoidValve, 10);
    itoa(instr.errorCode, errorCode, 10);

    p2.addTag("reason", instr.reason);
    p2.addTag("pump", instr.pumpModel);
    p2.addTag("solenoidValve", solenoidValve);
    p2.addField("allocatedWater", instr.allocatedWater);
    p2.addField("distributedWater", instr.distributedWater);
    // p2.addField("pumpTime", instr.pumpTime);
    p2.addTag("errorCode", errorCode);
    InfluxHelper::writeDataPoint(p2); // Write to Buffer

    return true;
}

void Irrigation::clearInstructions()
{
    instructions.clear();
}

void Irrigation::printError(uint8_t errorCode)
{
    Serial.print("Reason: ");
    Serial.println(errors[errorCode]);
}

/*
Report to MongoDB
https://arduinojson.org/v6/api/json/serializejson/
*/
bool Irrigation::reportToMongo(std::vector<Instruction> &instructions)
{
    StaticJsonDocument<1024> reports;
    String output;

    for (auto const &instr : instructions)
    {
        JsonObject report;
        report["reason"] = instr.reason;
        report["pump"] = instr.pumpModel;
        report["solenoidValve"] = instr.solenoidValve;
        report["allocatedWater"] = instr.allocatedWater;
        report["distributedWater"] = instr.distributedWater;
        // report["timeStamp"] = ...
        report["pumpTime"] = instr.pumpTime;
        report["errorCode"] = instr.errorCode;
        reports.add(report);
    }

    // output = "{}"; // Empty Json Obj
    serializeJson(reports, output);
    Services::doPostRequest("/settings/report", output);

    clearInstructions();

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