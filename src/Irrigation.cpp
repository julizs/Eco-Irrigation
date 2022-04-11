#include "Irrigation.h"

bool Irrigation::didEvaluate = false;
int Irrigation::waterLimit2h = 500;
int Irrigation::waterLimit24h = 5000;
std::vector<SolenoidMl> Irrigation::solenoidsMl;
FluxQueryResult Irrigation::cursor2h("empty");
FluxQueryResult Irrigation::cursor24h("empty");

/*
Do Query only once and not per SolenoidValve
Call from 2nd Core?
Check Query in Console, "influx query"
*/
FluxQueryResult Irrigation::recentIrrigations(uint8_t timePeriod)
{
    char query[200] = "";
    sprintf(query, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                   "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"pumpedWaterML\")"
                   "|> sum()",
            timePeriod);
    Serial.println(query);

    FluxQueryResult cursor = influxHelper.doQuery(query);
    return cursor;
}

void Irrigation::writeVector(FluxQueryResult &cursor)
{
    while (cursor.next())
    {
        uint8_t solenoidValve = cursor.getValueByName("solenoidValve").getLong();
        uint16_t waterAmount = cursor.getValueByName("_value").getLong();
        bool exists = false;

        for(auto &s: solenoidsMl)
        {
            if(s.solenoidValve == solenoidValve)
            {
                s.waterAmountMl += waterAmount;
                exists = true;
            }
        }
        if(!exists)
        {
            SolenoidMl sol;
            sol.solenoidValve = solenoidValve;
            sol.waterAmountMl = waterAmount;
            solenoidsMl.push_back(sol);
        }
        
        if (cursor.getError() != "")
        {
            Serial.println(cursor.getError());
        }
    }
}

/*
Gets called by Irrigation Algo and at Button Press
Button Evaluation in Realtime
Also check per Pump (multiple solenoidValves, Addition)
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve, uint16_t waterLimit)
{
    for(auto const &s : solenoidsMl)
       {
           if(s.solenoidValve == solenoidValve)
           {
               if(s.waterAmountMl > waterLimit)
               {
                   return false;
               }
           }
           // Serial.println(s.solenoidValve);
           // Serial.println(s.waterAmountMl);
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
void Irrigation::decideIrrigation()
{
    char message[64];
    didEvaluate = false;
    uint8_t waterNeeds = 1, lightNeeds = 1, plantSize = 1; // e.g. plantSize 1 = 100-200mm

    // Get Data once
    DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/sensors");
    DynamicJsonDocument plantNeeds = Services::doJSONGetRequest("/plants/needs");
    // FluxQueryResult cursor = recentIrrigations(2);
    // writeVector(cursor);

    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"];
        int solenoidValve = plants[i]["solenoidValve"].as<int>();
        
        if (!validSolenoid(solenoidValve, waterLimit24h)) // Efficient Checks
        {
            continue; // skip Plant
        }

        for (int j = 0; j < plantNeeds.size(); j++)
        {
            String name = plantNeeds[j]["name"];
            if (plantName.equals(name))
            {
                waterNeeds = plantNeeds[j]["waterNeeds"];
                lightNeeds = plantNeeds[j]["lightNeeds"];
                plantSize = plantNeeds[j]["plantSize"];
            }

            sprintf(message, "%s Needs: Water: %d, Light: %d, Plant Size: %d", name, waterNeeds, lightNeeds, plantSize);
            Serial.println(message);
        }
    }

    actions.add(&pump1);
    actions.add(&pump2);
    actions.add(&pump1);

    // [["Thymian", 350], ["Aloe",280]]
    // [[1,280]] (both on same Solenoid/relaisChannel)
    // [[1,3.7]] (pumpTime based on L and % Pwm and L/h of pump)

    didEvaluate = true;
}

void Irrigation::getIrrigationInfo(uint8_t solenoidValve, int irrigationAmount)
{
    // Access Tables from EEPROM or do Requests if needed
    DynamicJsonDocument pumps = Services::doJSONGetRequest("/pumps");

    // Get specific Pump

    /*
    int litersPerHour = pump["flowRate"][1];
    int pwmChannel = pump["pwmChannel"];

    if (litersPerHour == 0) // avoid DivideByZero
    {
        litersPerHour = 300;
    }

    float pumpTime = (irrigationAmount / (litersPerHour * 1000.0f)) * 3600;

    // Serial.print("Irrigate: "); Serial.println(plantGroup);
    Serial.println(pwmChannel);
    Serial.println(pumpTime);
    */

    // Output: [relaisChannel, pumpTime]
    // [[0, 4.6][1, 3.9]]
    // [pump1, 4.6]
}

/*
Do only 1 InfluxDB Request
*/
/*
void Irrigation::validSolenoids(bool validSolenoids[])
{
    int waterLimit1h = 500, waterLimit24h = 1000;
    int i = 0;

    FluxQueryResult cursor = influxHelper.doQuery(query);
    while (cursor.next())
    {
        int pumpedWaterML = cursor.getValueByName("_value").getLong();
        Serial.println(pumpedWaterML);
    }
    cursor.close();

    while (cursor.next())
    {
        uint8_t solenoidValve = cursor.getValueByName("_value").getLong();
        int pumpedWaterML = cursor.getValueByName("pumpedWaterML").getLong();
        if (pumpedWaterML > waterLimit1h)
        {
            validSolenoids[i] = false;
        }
        else
        {
            validSolenoids[i] = true;
        }
        i++;
    }

    if (cursor.getError() != "")
    {
        Serial.print("Query result error: ");
        Serial.println(cursor.getError());
    }
}
*/

/*
char query[32] = "/";
strcat(query, plantName);
strcat(query, "/needs");
DynamicJsonDocument needs = Services::doJSONGetRequest(query);
int currentSize = needs[0]["plantSize"];
Serial.println(currentSize);
*/