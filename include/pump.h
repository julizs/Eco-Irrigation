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
    class PumpModel
    {
        friend class Pump;

    public:
        char *name;
        byte minVoltage;
        byte maxVoltage;
        byte maxPumpingDuration;
        int flowRate;
        PumpModel();
        PumpModel(byte minVoltage, byte maxVoltage, byte maxPumpingDuration, int flowRate)
        {
            this->minVoltage = minVoltage;
            this->maxVoltage = maxVoltage;
            this->maxPumpingDuration = maxPumpingDuration;
            this->flowRate = flowRate;
        }
        byte getminVoltage()
        {
            return this->minVoltage;
        }
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