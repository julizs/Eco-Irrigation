#ifndef plant_h
#define plant_h
#include <main.h>
#include <ambientLight.h>
#include <soilmoisture.h>
#include <string>

class Plant
{
public:
    //char* name[50]; 
    String name; // Tag für InfluxDB
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
    bool setName(String name);
    bool measureSensors();
    bool writeToInflux();
};

#endif //plant.h