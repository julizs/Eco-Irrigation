#include "Irrigation.h"

const char query[] =   "from (bucket: \"messdaten\")"
                        "|> range(start: -24h)"
                        "|> filter(fn: (r) => r._measurement == \"Environment Data\" and r._field == \"rssi\""
                        " and r.device == \"ESP32\")";

// "and r.solenoidValve == \"1\""
const char query2[] =   "from (bucket: \"messdaten\")"
                        "|> range(start: -24h)"
                        "|> filter(fn: (r) => r._measurement == \"Irrigations\" and r._field == \"pumpedWaterML\")"
                        "|> sum()";


/* Irrigation Algorithm
  1.0 Consider Properties of each Plant in PlantGroup (Size, Dry Roots, ...)
  1.1 Consider recent Irrigations (InfluxDB) of this PlantGroup (e.g. less than 1l today, less than 0.5l in past 4 hours)
  1.2 Consider soilMoisture of every Plant from the PlantGroup
  2. Decide Subject (Plant/PlantGroup at Solenoid) and Milliliters
  3. Create (Irrigation) Action Obj that are executed sequential (one after another)
  4. Get necessary Info for Action (PumpModel, PumpTime, RelaisChannel, ...)
  5. Run Pump State Machine Loop(s)
  5. Create Irrigation Datapoint for InfluxDB (also if Irrigation failed)
  (Reason for Irrigation: ... , Irrigation Amount: ..., Reason for Failure: ...)
*/
void Irrigation::decideIrrigation()
{
    /*
    // Access Tables from EEPROM or do Requests if needed
    // DynamicJsonDocument plantNeeds = Utilities::readDoc(0, 1024);
    DynamicJsonDocument plants = Utilities::readDoc(3072, 2048);
    if (plants.isNull())
    {
        DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/json");
    }
    */

    // Do InfluxDB query once and convert to Array once
    FluxQueryResult recentIrrigations = influxHelper.doQuery(query2);
    while (recentIrrigations.next())
    {
        int pumpedWaterML = recentIrrigations.getValueByName("_value").getLong();
        Serial.println(pumpedWaterML);
    }
    recentIrrigations.close();

    int result[50];
    // convert(recentIrrigations, result);
    for(int i = 0; i < sizeof(result)/sizeof(result[0]); i++)
    {
        // Serial.println(result[i]);
    }

/*
for (int i = 0; i < plants.size(); i++)
{
    String plantName = plants[i]["name"].as<String>();
    Serial.println(plantName);

    // 1. Check recentIrrigations per Solenoid/relaisChannel
    // continue (skip Evaluation of this Plant) if over Threshold
    uint8_t relaisChannel = plants[plantName]["solenoidValve"];
    if (!validSolenoid(recentIrrigations, relaisChannel))
    {
        continue;
    }

    // 2. If recentIrrigations under Threshold, get Plant Needs
    // Compare Needs with recent Irrigations
    char message[100];
    uint8_t lightNeeds = plants[plantName]["lightNeeds"];
    uint8_t waterNeeds = plants[plantName]["waterNeeds"];
    uint8_t plantSize = plants[plantName]["plantSize"];
    sprintf(message, "Light Needs: %d, Water Needs: %d", "Plant Size: %d", lightNeeds, waterNeeds, plantSize);
    Serial.println(message);
}
*/
/*
wateringNeeded = true;
Pump *action1 = &pump1;
Pump *action2 = &pump1;
actions.add(action1);
actions.add(action2);
*/

// Output: [Plant, Milliliters]
// [["Thymian", 350], ["Aloe",280]]
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

bool Irrigation::validSolenoid(FluxQueryResult &cursor, uint8_t relaisChannel)
{
    int pumpedWaterML;
    int limit = 500; // ML, per 24h

    while (cursor.next())
    {
        uint8_t channel = cursor.getValueByName("relaisChannel").getLong();
        if (relaisChannel == channel)
        {
            pumpedWaterML = cursor.getValueByName("pumpedWaterML").getLong();
            if (pumpedWaterML > limit)
            {
                return false;
            }
            Serial.print("Water Amount on Solenoid ");
            Serial.print(channel);
            Serial.print(" : ");
            Serial.print(pumpedWaterML);
        }
    }

    if (cursor.getError() != "")
    {
        Serial.print("Query result error: ");
        Serial.println(cursor.getError());
        return false;
    }

    // No Irrigations on this relaisChannel
    return true;
}