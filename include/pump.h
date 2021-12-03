#ifndef pump_h
#define pump_h
#include <Arduino.h>
#include <pins.h>

enum PumpState
{
    IDLE,
    ON
};

class Pump
{
public:
    struct PumpModel
    {
        friend class Pump;
        char *name;
        byte minVoltage;
        byte maxVoltage;
        byte maxPumpingDuration;
        int measuredFlowRate;
        byte getminVoltage();
    };

    PumpModel pumpModel;
    PumpState state;
    bool doPump;
    unsigned long stateBeginMillis;

    Pump(PumpModel pumpModel);
    void Update();

private:
    void switchOn();
    void switchOff();
    void calibrateFlowRate();
};

#endif // pump_h