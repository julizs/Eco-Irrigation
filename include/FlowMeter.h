#ifndef FlowMeter_h
#define FlowMeter_h
#include <main.h>

struct FlowData
{
    float waterVolume, waterLiterPerMinute;
    double flowLperMin, flowLperHour, flowMlperSec; // Liters per Minute/Hour
    unsigned long pulses, amountMl;
};

// aka Hall-effect Sensor
class FlowMeter
{
    public:
    FlowData measurement;
    uint8_t pinNum;  

    FlowMeter(uint8_t pinNum);
    void setup();
    void pulse();
    FlowData measureFlow();
    void measureVolume();
    void writePoint();

    private:
    // unsigned long pulses;
    // unsigned long amountMl;
};

#endif // FlowMeter_h