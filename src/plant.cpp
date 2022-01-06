#include "plant.h"


//Plant::Plant(Fotoresistor &f): lightSensor(f){}

Plant::Plant(AmbientLight &l, SoilMoisture &s): lightSensor(l)
{
    soilMoistureSensors.push_back(s);
}

bool Plant::measureSensors()
{
    bool measurementsSuccessful;

    // Plant has 0-3 Soil Moisture Sensors
    if(soilMoistureSensors.size() > 0)
    {
        for(auto& sMS: soilMoistureSensors) {
        int soilMoisture = sMS.measureSoilMoistureSmoothed();
        Serial.print(name);
        Serial.print(" soil moisture: ");
        Serial.println(soilMoisture);
        }
    }
    
    return measurementsSuccessful;
}

bool Plant::setName(String newName)
{
    name = newName;
}

bool Plant::writeToInflux()
{
    return true;
}