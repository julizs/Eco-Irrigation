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
    PowerData data;

    uint16_t measureIntervall, currentTime, lastTime;

    PowerMeter(TwoWire &wire);

    bool setup();
    bool isReady();

    PowerData measure();
    bool measureAndSubmit(); 
    bool measurementValid(const PowerData &measurement);

    private:
    TwoWire &wire;
    bool writePoint(const PowerData &measurement);
    void printMeasurement(const PowerData &measurement);
};

#endif // PowerMeter_h