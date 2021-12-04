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
        char *name[50];
        byte minVoltage;
        byte maxVoltage;
        byte maxPumpingDuration;
        int flowRate;
        PumpModel(byte, byte, byte, int);
        byte getminVoltage();
    };

    PumpModel pumpModel; // PumpModel& pumpModel
    PumpState currentState, lastState;
    bool doPump;
    unsigned long stateBeginMillis;

    Pump(PumpModel& pumpModel); // Konstr.
    const PumpModel& getPumpModel() const;
    void setPumpModel(const Pump::PumpModel& pM);
    void setup();
    void loop();

private:
    void switchOn();
    void switchOff();
    void calibrateFlowRate();
};

#endif // pump_h