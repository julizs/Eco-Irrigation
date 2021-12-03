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
    // Nested Class Declaration
    class PumpModel
    {
        friend class Pump;
    public:
        char *name;
        byte minVoltage;
        byte maxVoltage;
        byte maxPumpingDuration;
        int flowRate;
        PumpModel(byte, byte, byte, int);
        byte getminVoltage();
    };

    PumpModel& pumpModel;
    PumpState state;
    bool doPump;
    unsigned long stateBeginMillis;

    Pump(PumpModel& pumpModel); // Konstr.
    //const PumpModel& Pump::GetPumpModel();
    void Update();

private:
    void switchOn();
    void switchOff();
    void calibrateFlowRate();
};

#endif // pump_h