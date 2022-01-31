#ifndef SoilMoisture_h
#define SoilMoisture_h
#include <main.h>
#include <multiplexer.h>

class SoilMoisture
{
public:
    
    static int measureSoilMoistureSmoothed(int pinNum);
    static int measureSoilMoisture(int pinNum);
    float voltageToPercentage(int constrainedValue);
};

#endif // SoilMoisture_h