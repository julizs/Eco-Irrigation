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

    Serial.println(query);

    FluxQueryResult cursor = influxHelper.doQuery(query);
    while (cursor.next())
    {
        pumpedWaterML = cursor.getValueByName("_value").getLong();
        Serial.println(pumpedWaterML);
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
    int waterLimit1h = 500, waterLimit24h = 1000;
    int pumpedWater1h = recentIrrigations(1, solenoidValve);
    int pumpedWater24h = recentIrrigations(24, solenoidValve);

    if(pumpedWater1h > waterLimit1h && pumpedWater24h > waterLimit24h)
    {
        return false;
    }
    else if (pumpedWater1h == -1 || pumpedWater24h == -1)
    {
        return false;
    }
    
    return false;
}

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
    didEvaluate = false;
    // Access Tables from EEPROM
    // DynamicJsonDocument plants = Utilities::readDoc(0, 1024);
    // DynamicJsonDocument plantNeeds = Utilities::readDoc(1024, 1024);
    DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/sensors");
    DynamicJsonDocument plantNeeds = Services::doJSONGetRequest("/plants/needs");
    

    for (int i = 0; i < plantNeeds.size(); i++)
    {
        char message[100];
        int waterNeeds = plantNeeds[i]["waterNeeds"].as<int>();
        int lightNeeds = plantNeeds[i]["lightNeeds"].as<int>();
        int currentSize = plantNeeds[i]["currentSize"].as<int>();
        sprintf(message, "Water Needs: %d", "Light Needs: %d", "Plant Size: %d", waterNeeds, lightNeeds, currentSize);
        Serial.println(message);
    }
    /*
    for (int i = 0; i < plants.size(); i++)
    {
        String plantName = plants[i]["name"].as<String>();
        Serial.println(plantName);

        // 1. Check if Irrigation necessary
        char message[100];
        uint8_t lightNeeds = plantNeeds[plantName]["lightNeeds"].as<int>();
        uint8_t waterNeeds = plantNeeds[plantName]["waterNeeds"].as<int>();
        uint8_t plantSize = plantNeeds[plantName]["plantSize"].as<int>();
        sprintf(message, "Light Needs: %d, Water Needs: %d", "Plant Size: %d", lightNeeds, waterNeeds, plantSize);
        Serial.println(message);

        // 2. Reduce to relaisChannels and check if Solenoid valid
        uint8_t relaisChannel = plants[plantName]["solenoidValve"];
        int waterAmound1h = recentIrrigations(1,0); // range -1h, relaisChannel 0
        int waterAmount24 = recentIrrigations(24,0);
        if(!validSolenoid(relaisChannel))
        {
            Serial.println("FAIL");
        }
    }
    */

    // Pump *action1 = &pump1;
    // Pump *action3 = &pump1;
    // Pump *action2 = &pump2;
    actions.add(&pump1);
    actions.add(&pump2);
    actions.add(&pump1);

    // Output: [Plant, Milliliters]
    // [["Thymian", 350], ["Aloe",280]]

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