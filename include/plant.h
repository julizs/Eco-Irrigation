#ifndef plant_h
#define plant_h
#include <main.h>
#include <ambientLight.h>
#include <soilmoisture.h>
#include <influxHelper.h>
#include <string>

class Plant
{
public:
    //char* name[50]; 
    String name; 
    AmbientLight lightSensor;
    std::vector <SoilMoisture> soilMoistureSensors;
    int irLight, visibleLight;
    std::vector <int> soilMoistureValues;

    //Plant();
    //Plant(Fotoresistor& lightSensor);
    Plant(AmbientLight& lightSensor, SoilMoisture& soilMoistureSensor);
    bool addSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool removeSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool setName(String name);
    bool measureSensors();
    bool writeToInflux();
};

#endif //plant.h