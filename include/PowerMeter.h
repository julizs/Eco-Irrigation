#ifndef PowerMeter_h
#define PowerMeter_h
#include <main.h>
#include "Adafruit_INA219.h"

struct PowerData
{
    float busVoltage;
    float shuntVoltage;
    float voltage;
    float current;
    float power;
    float energy;
};

// aka Hall-effect Sensor
class PowerMeter
{
    public: 
    Adafruit_INA219 ina219; 
    uint16_t measureIntervall;
    unsigned long currentTime;
    unsigned long lastTime;
    float voltage_V, shuntVoltage_mV, busVoltage_V;
    float current_mA, power_mW;

    PowerMeter(TwoWire &wire);
    bool setupIna();
    bool inaReady();
    PowerData measureIna();
    void writePoint();

    private:
    TwoWire &wire;
};

#endif // PowerMeter_h