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

    // For reapeating Measurements in Loop
    uint16_t measureIntervall;
    unsigned long currentTime;
    unsigned long lastTime;

    // Interchangable by User
    DynamicJsonDocument pumpModel;

    // Set by Irrigation Algo
    int8_t relaisChannel;
    float pumpTime;

    int pwmPin, pwmChannel, frequency, resolution, dutyCycle;

    Cistern &cistern;

    Pump(int pwmChannel, int pwmPin, Cistern &cistern);
    void setup();
    virtual void loop();

    void switchOff();
    virtual bool isDone();
    virtual void resetMachine();
    void add_callback(callback act);

private:
    void setupPWM();
    void switchOn();
    bool checkPumpPerformance(unsigned short);
    void commonStateLogic();
    bool countTime(int durationSec);
    callback setupToFs;
};

extern Pump pump1, pump2;

#endif // Pump_h