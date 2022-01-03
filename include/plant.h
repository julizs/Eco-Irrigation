#ifndef plant_h
#define plant_h
#include <main.h>
#include <fotoresistor.h>
#include <soilmoisture.h>

class Plant
{
public:
    char* name[50]; // Tag für InfluxDB
    Fotoresistor lightSensor;
    std::vector <SoilMoisture> soilMoistureSensors;

    //Plant();
    Plant(Fotoresistor& lightSensor);
    Plant(Fotoresistor& lightSensor, SoilMoisture& soilMoistureSensor);
    bool addLightSensor(Fotoresistor lightSensor);
    bool removeLightSensor(Fotoresistor lightSensor);
    bool addSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool removeSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool measureSensors();
    bool writeToInflux();
};

#endif //plant.h