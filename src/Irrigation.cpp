#include "Irrigation.h"

/* Irrigation Algorithm
Decides Subjects (plant or plantGroup) and PumpTime in Milliliters
Creates Actions that are executed sequential (one Pump State Machine Loop after another)
*/
/* Irrigation Algorithm
  Consider recent Irrigations of PlantGroup and needs of each Plants inside PlantGroup (e.g. Moisture Need, Size of Plant, ...)
  (all Plants that are affected of this Irrigation need to be considered)
  1.1 (Quick) Get PlantGroup Table, check recent Irrigations (InfluxDB) of this Group (e.g. less than 1l today, less than 0.5l in past 4 hours)
  1.2 (Long) Get every single Plant from the PlantGroup and check soilMoisture (do this in State 1 already, with 2nd Core ?)
  3. Check Waterlevel in Tank, if not enough then create failed Irrigation InfluxDB datapoint
  (Reason for Irrigation: ... , Irrigation Amount: ..., Reason for Failure: ...)
*/
void Irrigation::decideIrrigation()
{

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