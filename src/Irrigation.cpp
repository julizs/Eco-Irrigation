#include "Irrigation.h"

bool Irrigation::didEvaluate = false;

const char query[] = "from (bucket: \"messdaten\")"
                     "|> range(start: -24h)"
                     "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"pumpedWaterML\")"
                     "|> sum()";

/*
Check Irrigation Limits per solenoidValve/relaisChannel for given time Period
*/
int Irrigation::recentIrrigations(uint8_t timePeriod, uint8_t solenoidValve)
{
    int pumpedWaterML = 0;
    char query[200] = "";

    sprintf(query, "from (bucket: \"messdaten\")|> range(start: -%dh)"
                   "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"pumpedWaterML\""
                   "and r.solenoidValve == \"%d\") |> sum()",
            timePeriod, solenoidValve);
    // Serial.println(query);

    FluxQueryResult cursor = influxHelper.doQuery(query);
    while (cursor.next())
    {
        pumpedWaterML = cursor.getValueByName("_value").getLong();
        Serial.print(pumpedWaterML);
        Serial.print(" Ml released by Solenoid ");
        Serial.println(solenoidValve);
    }

    if (cursor.getError() != "")
    {
        pumpedWaterML = -1;
        Serial.println(cursor.getError());
    }

    cursor.close();
    return pumpedWaterML;
}

/*
Gets called by Irrigation Algo and Pump StateMachine at Button Press
*/
bool Irrigation::validSolenoid(uint8_t solenoidValve)
{
    int waterLimit = 500; // Milliliters
    int pumpedWater = recentIrrigations(2, solenoidValve);

    if (pumpedWater > waterLimit || pumpedWater == -1)
    {
        return false;
    }
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
void Irrigation::decideIrrigation()
{
    didEvaluate = false;
    // Do 1 Request each only, or read from EEPROM
    // DynamicJsonDocument pumps = Utilities::readDoc(1024, 2048);
    DynamicJsonDocument pumps = Services::doJSONGetRequest("/pumps/json");
    DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/sensors");
    DynamicJsonDocument plantNeeds = Services::doJSONGetRequest("/plants/needs");
    // DynamicJsonDocument plants = Utilities::readDoc(0, 1024);
    // DynamicJsonDocument plantNeeds = Utilities::readDoc(1024, 1024);

    // 1. Iterate unique SolenoidValves in System
    for (int i = 0; i < pumps.size(); i++)
    {
        String pumpModel = pumps[i]["name"];
        int relaisChannel, pumpedWater48h;
        // Serial.println(pumpModel);

        // New way since ArduinoJson 6
        // StaticJsonDocument<64> doc;
        JsonArray array = pumps[i]["solenoidValve"];
        if (array.isNull())
        {
            // Serial.println("No connected Solenoid Valves.");
            continue;
        }
        for (int j = 0; j < array.size(); j++)
        {
            relaisChannel = array[j];
            // Serial.print("Connected Solenoid Valves: ");
            // Serial.println(relaisChannel);

            // Do only 1 InfluxDB Request per Solenoid Valve
            pumpedWater48h = recentIrrigations(48, relaisChannel);

            decidePlants(relaisChannel, pumpedWater48h, plants, plantNeeds);
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

/*
Check each Plant connected to same Solenoid Valve
*/
void Irrigation::decidePlants(uint8_t solenoidValve, int recentWater, DynamicJsonDocument &plants, DynamicJsonDocument &plantNeeds)
{
    // Only 2 MongoDB Requests, but lots of iterating
    for (int i = 0; i < plants.size(); i++)
    {
        int relaisChannel = plants[i]["solenoidValve"].as<int>();
        if (relaisChannel == solenoidValve)
        {
            String plantName = plants[i]["name"];
            // Serial.println(plantName);
            for (int j = 0; j < plantNeeds.size(); j++)
            {
                String name = plantNeeds[j]["name"];
                if (plantName.equals(name)) // strcmp(plantName, name) == 0
                {
                    Serial.println(plantName);
                    uint8_t waterNeeds = plantNeeds[j]["waterNeeds"];
                    uint8_t lightNeeds = plantNeeds[j]["lightNeeds"];
                    int plantSize = plantNeeds[j]["plantSize"];
                    char message[64] = "";
                    sprintf(message, "Water Needs: %d, Light Needs: %d, Plant Size: %d", waterNeeds, lightNeeds, plantSize);
                    Serial.println(message);
                }
            }
            /*
            char query[32] = "/";
            strcat(query, plantName);
            strcat(query, "/needs");
            DynamicJsonDocument needs = Services::doJSONGetRequest(query);
            int currentSize = needs[0]["plantSize"];
            Serial.println(currentSize);
            */
        }
    }

    /*
    if (!validSolenoid(solenoidValve))
    {
        continue;
    }
    */
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