#ifndef SoilMoisture_h
#define SoilMoisture_h

#include <main.h>
#include <Services.h>
#include <multiplexer.h>

class SoilMoisture
{
public:
    static void measurePlant(JsonArray &moistureSensors, Point &p);   
    static int measureSoilMoisture(int pinNum);
    static int measureSoilMoistureSmoothed(int pinNum);
    static void setSensorRanges();
    static void writePoint(Point &p);
    
private:
    static DynamicJsonDocument sensorRanges;
    static void getSensorRange(int pinNum, int range[]);
    static int voltageToPercentage(int pinNum, int rawValue);
};

#endif // SoilMoisture_h