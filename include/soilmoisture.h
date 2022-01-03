#ifndef soilmoisture_h
#define soilmoisture_h
#include <main.h>
#include <multiplexer.h>

class SoilMoisture
{
public:
    // Individueller "Floor" jedes Moisture Sensors 
    // (max. Feuchtigkeit = ca. 400-550 statt 0)
    int sensorFloor;
    byte sampleSize;
    byte multiplexerPin; // Never changes, only Association from Sensor to Plant Object

    SoilMoisture(int sensorFloor, byte sampleSize, byte multiplexerPin);

    int measureSoilMoistureSmoothed();
    int measureSoilMoisture();
    float calcPercentage(int constrainedValue);
};

#endif //soilmoisture_h