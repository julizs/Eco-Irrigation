#ifndef FlowMeter_h
#define FlowMeter_h
#include <main.h>

struct flowData
{
    float waterVolume;
    float waterLiterPerMinute;
};

// aka Hall-effect Sensor
class FlowMeter
{
    public:
    
    uint8_t pinNum;
    uint16_t measureIntervall;
    double flowLperM, flowLperH, flowMlperSec; // Liters per Minute/Hour
    unsigned long currentTime;
    unsigned long lastTime;
    unsigned long pulse_freq;
    unsigned long amountMl;

    FlowMeter(uint8_t pinNum);
    void setup();
    void pulse();
    void measureVolume();
    void measureFlow();
};

#endif // FlowMeter_h