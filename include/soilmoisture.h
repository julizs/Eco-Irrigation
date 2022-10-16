#ifndef SoilMoisture_h
#define SoilMoisture_h

#include <main.h>
#include <Services.h>
#include <multiplexer.h>

class SoilMoisture
{
public:
    static DynamicJsonDocument sensorRanges;
    
    static void measurePlant(JsonArray &moistureSensors, Point &p);
    static void setSensorRanges();
    static int measureSoilMoistureSmoothed(int pinNum);
    static int measureSoilMoisture(int pinNum);
    static void writePoint(Point &p);
    
private:
    static void getSensorRange(int pinNum, int range[]);
    static int voltageToPercentage(int pinNum, int rawValue);
};

#endif // SoilMoisture_h