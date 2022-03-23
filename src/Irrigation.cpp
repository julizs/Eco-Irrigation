#include "Irrigation.h"


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

    wateringNeeded = true;

    // pump1.prepareIrrigation("succulents", 350);
    // pump1.prepareIrrigation("vegetables", 550);
    // pump1.prepareIrrigation("Tomate", 150);
}


void Irrigation::getSolenoidInfo(const char *irrigationSubject, int irrigationAmount)
{
    // 2. Get Solenoid (Relais Channel), SolenoidState (Relais State) and Pump Name
    DynamicJsonDocument solenoids = Services::doJSONGetRequest("/solenoidValves");

    bool relaisOpen;

    // For every Solenoid
    for (int i = 0; i < solenoids.size(); i++)
    {
        const char *plantGroupOpen = solenoids[i]["open"];
        const char *plantGroupClosed = solenoids[i]["closed"];

        // Check if PlantGroup is attached to this Solenoid (and which State)
        if (strcmp(plantGroupOpen, irrigationSubject) == 0)
        {
            relaisOpen = true;
        }
        else if (strcmp(plantGroupClosed, irrigationSubject) == 0)
        {
            relaisOpen = false;
        }
        else
        {
            continue; // Skip rest of Code, goto next Solenoid
        }

        const char *pumpName = solenoids[i]["pump"];

        // Per Plant Group:
        int relaisChannel = solenoids[i]["relais"].as<int>() - 1;
        Serial.print(relaisChannel); Serial.println(relaisOpen);

        // 2. Drive Pump
        getPumpInfo(pumpName, irrigationAmount);
    }
}

void Irrigation::getPumpInfo(const char* pumpName, int irrigationAmount)
{
    char endpoint[50] = "/pumps";
    strcat(endpoint, pumpName);
    DynamicJsonDocument pump = Services::doJSONGetRequest(endpoint);

    int litersPerHour = pump["flowRate"][1].as<int>();
    int pwmChannel = pump["pwmChannel"].as<int>();

    if (litersPerHour == 0) // avoid DivideByZero
    {
        litersPerHour = 300;
    }

    float pumpTime = (irrigationAmount / (litersPerHour * 1000.0f)) * 3600;

    // Serial.print("Irrigate: "); Serial.println(plantGroup);
    Serial.println(pwmChannel); Serial.println(pumpTime);
}