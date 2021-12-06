#include "plant.h"


//Plant::Plant(Fotoresistor &f): lightSensor(f){}

Plant::Plant(Fotoresistor &f, SoilMoisture &s): lightSensor(f)
{
    soilMoistureSensors.push_back(s);
}

bool Plant::measureSensors()
{
    bool measurementsSuccessful;

    // Get lightValue from plantGroup!
    float lightValue = lightSensor.measureLightSmoothed();

    // Plant has 0-3 sMS
    if(soilMoistureSensors.size() > 0)
    {
        for(auto& sMS: soilMoistureSensors) {
        float soilMoisture = sMS.measureSoilMoistureSmoothed();
        }
    }
    
    return measurementsSuccessful;
}

bool Plant::writeToInflux()
{
    return true;
}