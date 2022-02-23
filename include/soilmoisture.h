#ifndef SoilMoisture_h
#define SoilMoisture_h
#include <main.h>
#include <multiplexer.h>

class SoilMoisture
{
public:
    
    static int measureSoilMoistureSmoothed(int pinNum);
    static int measureSoilMoisture(int pinNum);
    static void getSensorRange(int pinNum, int range[]);
    static int voltageToPercentage(int pinNum, int rawValue);
};

#endif // SoilMoisture_h