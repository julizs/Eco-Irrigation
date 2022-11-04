#ifndef PowerMeter_h
#define PowerMeter_h
#include <main.h>
#include "Adafruit_INA219.h"

struct PowerData
{
    float voltage, busVoltage, shuntVoltage;
    float current, power, energy;
};

// aka Hall-effect Sensor
class PowerMeter
{
    public: 
    Adafruit_INA219 ina219; 
    PowerData measurement;

    uint16_t measureIntervall, currentTime, lastTime;

    PowerMeter(TwoWire &wire);
    
    bool setupIna();
    bool inaReady();

    PowerData measureIna();
    void writePoint();

    private:
    TwoWire &wire;
};

#endif // PowerMeter_h