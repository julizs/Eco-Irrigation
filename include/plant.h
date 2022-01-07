#ifndef plant_h
#define plant_h
#include <main.h>
#include <ambientLight.h>
#include <soilmoisture.h>
#include <influxHelper.h>

class Plant
{
public:
    char name[50];
    AmbientLight lightSensor;
    std::vector <SoilMoisture> soilMoistureSensors;
    int irLight, visibleLight;
    std::vector <int> soilMoistureValues;

    //Plant();
    //Plant(Fotoresistor& lightSensor);
    Plant(AmbientLight& lightSensor, SoilMoisture& soilMoistureSensor);
    bool addSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool removeSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool setName(char newName[]);
    bool measureSensors();
    bool writeToInflux();
};

#endif //plant.h