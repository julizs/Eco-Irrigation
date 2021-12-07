#ifndef pump_h
#define pump_h
#include <Arduino.h>
#include <pins.h>

enum class PumpState
{
    IDLE,
    ON,
    TURNING_ON,
    TURNING_OFF
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
    const char *stateNames[2] = {"PUMP_IDLE", "PUMP_ON"};
    unsigned long stateBeginMillis;
    bool pumpSignal;

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