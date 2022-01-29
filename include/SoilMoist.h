#ifndef soilMoist_h
#define soilMoist_h
#include <main.h>
#include <multiplexer.h>

class SoilMoist
{
public:
    
    static int measureSoilMoistureSmoothed(int pinNum);
    static int measureSoilMoisture(int pinNum);
    float voltageToPercentage(int constrainedValue);
};

#endif //soilMoist_h