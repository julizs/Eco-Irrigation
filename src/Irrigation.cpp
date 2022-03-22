#include "Irrigation.h"

/* Irrigation Algorithm
Decides Subjects (plant or plantGroup) and PumpTime in Milliliters
Creates Actions that are executed sequential (one Pump State Machine Loop after another)
*/
void Irrigation::decideIrrigation()
{

}


void Irrigation::getSolenoidInfo(const char *irrigationSubject, int irrigationAmount)
{
    // 2. Get Solenoid (Relais Channel), SolenoidState (Relais State) and Pump Name
    char url[50] = "";
    strcat(url, baseUrl);
    strcat(url, "/solenoidValves");
    DynamicJsonDocument solenoids = services.doJSONGetRequest(url);

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
    char url[50] = "";
    strcat(url, baseUrl);
    strcat(url, "/pumps/");
    strcat(url, pumpName);
    DynamicJsonDocument pump = services.doJSONGetRequest(url);

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