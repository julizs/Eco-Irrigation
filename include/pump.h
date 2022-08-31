#ifndef Pump_h
#define Pump_h
#include <Cistern.h>
#include <Irrigation.h>
#include "Adafruit_INA219.h"

enum class PumpState
{
    IDLE = 0,
    ON,
    DONE,
    ABORT
};

struct INAdata
{
    float busVoltage;
    float shuntVoltage;
    float voltage;
    float current;
    float power;
    float energy;
};

class Pump : public ISubStateMachine
{
    using callback = void (*)(); // Func Pointer
public:
    PumpState currentState, lastState;
    char const *stateNames[4] = {"PUMP_IDLE", "PUMP_ON", "PUMP_DONE", "PUMP_ABORT"};
    char const *errors[6] = {"None", "ToF Setup failed.", "Too many recent Irrgations.",
                             "Waterlevel not sufficient.", "Irrigation cancelled by User.",
                             "Invalid SolenoidValve"};
    unsigned short errorCode, minStateDuration;
    unsigned long stateBeginMillis;

    // Interchangable by User
    DynamicJsonDocument pumpModel;

    // Set by Irrigation Algo
    int8_t relaisChannel;
    float pumpTime;

    int pwmPin, pwmChannel, frequency, resolution, dutyCycle;

    Cistern &cistern;
    Adafruit_INA219 ina219;
    FlowMeter meter;

    float voltage_V, shuntVoltage_mV, busVoltage_V;
    float current_mA, power_mW;

    Pump(int pwmChannel, int pwmPin, Cistern &cistern);
    void setup();
    virtual void loop();

    void switchOff();
    bool setupIna();
    bool inaReady();
    void add_callback(callback act);
    virtual bool isDone();
    virtual void resetMachine();

private:
    INAdata readIna();
    void writeIna();
    void switchOn();
    void setupPWM();
    bool checkPumpPerformance(unsigned short);
    bool countTime(int durationSec);
    void commonStateLogic();
    callback setupToFs;
};

extern Pump pump1, pump2;

#endif // Pump_h