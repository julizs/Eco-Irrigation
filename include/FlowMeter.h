#ifndef FlowMeter_h
#define FlowMeter_h
#include <main.h>

struct FlowData
{
    double flowLperMin, flowLperHour, flowMlperSec; // Liters per Minute/Hour
    uint16_t amountMl;
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
    uint16_t measureAmount();
    void writePoint();

    private:
    uint16_t pulses;
    // unsigned long pulses;
    // unsigned long amountMl;
};

#endif // FlowMeter_h