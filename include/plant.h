#ifndef plant_h
#define plant_h
#include <main.h>
#include <ambientLight.h>
#include <soilmoisture.h>

class Plant
{
public:
    char* name[50]; // Tag für InfluxDB
    //Fotoresistor lightSensor;
    AmbientLight lightSensor;
    std::vector <SoilMoisture> soilMoistureSensors;

    //Plant();
    //Plant(Fotoresistor& lightSensor);
    Plant(AmbientLight& lightSensor, SoilMoisture& soilMoistureSensor);
    //bool addLightSensor(Fotoresistor lightSensor);
    //bool removeLightSensor(Fotoresistor lightSensor);
    bool addSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool removeSoilMoistureSensor(SoilMoisture soilMoistureSensor);
    bool measureSensors();
    bool writeToInflux();
};

#endif //plant.h