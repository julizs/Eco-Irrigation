#include "plant.h"


//Plant::Plant(Fotoresistor &f): lightSensor(f){}
Plant::Plant(Fotoresistor &f, SoilMoisture &s): lightSensor(f), soilMoistureSensor(s){}

/*
Plant::Plant(Fotoresistor lightSensor, SoilMoisture soilMoistureSensor)
{
    this->lightSensor = lightSensor;
    soilMoistureSensors.push_back(soilMoistureSensor);
}
*/