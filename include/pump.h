#ifndef Pump_h
#define Pump_h
#include <main.h>
#include <Cistern.h>
#include <Irrigation.h>
#include "Adafruit_INA219.h"

class ISubStateMachine
{
    public:
        virtual ~ISubStateMachine() {}
        virtual bool isDone() = 0;
};

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
    using callback = void (*)(); //function pointer
public:
    PumpState currentState, lastState;
    char const *stateNames[4] = {"PUMP_IDLE", "PUMP_ON", "PUMP_DONE", "PUMP_ABORT"};
    char const *errors[5] = {"None", "ToF Setup failed.", "Network Request failed.",
                             "Water not sufficient.", "Irrigation aborted by User."};
    unsigned short errorCode, minStateDuration;
    unsigned long stateBeginMillis;

    DynamicJsonDocument pumpModel;
    int pwmPin, pwmChannel, frequency, resolution, dutyCycle;
    float pumpTime;

    Cistern cistern;
    Adafruit_INA219 ina219;
    float voltage_V, shuntVoltage_mV, busVoltage_V;
    float current_mA, power_mW;

    Pump(int pwmChannel, int pwmPin, Cistern &cistern);
    void setup();
    void loop();

    bool setupIna();
    INAdata readIna();
    void writeIna();
    void switchOff();
    void add_callback(callback act);
    virtual bool isDone();

private:
    void switchOn();
    void setupPWM();
    bool checkPumpPerformance(unsigned short);
    bool countTime(int durationSec);
    void commonStateLogic();
    callback setupToFs;
};

#endif // Pump_h