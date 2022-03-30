#include "Irrigation.h"

const char query[] = "from (bucket: \"messdaten\")"
                     "|> range(start: -24h)"
                     "|> filter(fn: (r) => r._measurement == \"Irrigations\""
                     " and r._field == \"pumpedWaterML\""
                     " and r.device == \"ESP32\")"
                     "|> min()";

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
    // Access Tables from EEPROM or do Requests if needed
    // DynamicJsonDocument recentIrrigations = Services::doJSONGetRequest("/irrigations");
    // DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/json");
    DynamicJsonDocument plants = Utilities::readDoc(0);
    DynamicJsonDocument plantNeeds = Utilities::readDoc(1024);

    // Do InfluxDB query once and convert to Array once
    FluxQueryResult recentIrrigations = influxHelper.doQuery(query);
    int result[4];
    /*
    if(recentIrrigations.isNull())
        {
            return;
        }
        else
        {
           result[] =
        }
    */

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

    wateringNeeded = true;
    // pump1.prepareIrrigation("succulents", 350);
    // pump1.prepareIrrigation("Tomate", 150);
}

void Irrigation::getIrrigationInfo(const char *plantName, int irrigationAmount)
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
}

void Irrigation::convert(FluxQueryResult &cursor, int result[])
{
    int i = 0;
    while (cursor.next())
    {
        uint8_t channel = cursor.getValueByName("relaisChannel").getLong();
        Serial.println(channel);
        int pumpedWaterML = cursor.getValueByName("pumpedWaterML").getLong();
        result[i] = pumpedWaterML;
        i++;
    }

    if (cursor.getError() != "")
    {
        Serial.print("Query result error: ");
        Serial.println(cursor.getError());
    }
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