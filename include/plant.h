#ifndef plant_h
#define plant_h
#include <pins.h>
#include <fotoresistor.h>
#include <soilmoisture.h>
#include <Arduino.h>

class Plant
{
public:
    char* name[50];
    Fotoresistor lightSensor;
    //SoilMoisture soilMoistureSensor;
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