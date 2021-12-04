#ifndef soilmoisture_h
#define soilmoisture_h
#include <Arduino.h>

class SoilMoisture
{
public:
    // Individueller "Floor" jedes Moisture Sensors 
    // (max. Feuchtigkeit = ca. 400-550 statt 0)
    int sensorFloor;
    byte sampleSize;

    SoilMoisture(int sensorFloor, byte sampleSize);

    float measureSoilMoistureSmoothed();
    float measureSoilMoisture();
};

#endif //soilmoisture_h