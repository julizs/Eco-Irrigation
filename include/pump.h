#ifndef Pump_h
#define Pump_h
#include <Cistern.h>
// #include <FlowMeter.h>
// #include <Irrigation.h>
// #include "Adafruit_INA219.h"

enum class PumpState
{
    INIT = 0,
    ON,
    DONE,
    ABORT
};

class Pump : public ISubStateMachine
{
    // Func Pointer to call Methode from Main
    using callback = void (*)();

public:
    PumpState currentState, lastState;
    char const *stateNames[4] = {"PUMP_INIT", "PUMP_ON", "PUMP_DONE", "PUMP_ABORT"};
    char const *errors[6] = {"None", "ToF Setup failed.", "Too many recent Irrgations.",
                             "Waterlevel not sufficient.", "Irrigation cancelled by User.",
                             "Invalid SolenoidValve"};
    
    bool isDone;
    uint8_t transCount, maxSelfTrans;
    uint8_t errorCode;
    uint32_t stateBeginMillis, minStateDuration;

    // For reapeating Measurements in Loop
    uint32_t measureIntervall, currentTime, lastTime;

    // Set by Irrigation Algo
    uint16_t allocatedWater;
    uint8_t relaisChannel;
    float pumpTime;

    // Pwm Control
    uint8_t pwmPin, resolution, pwmChannel;
    uint16_t frequency, dutyCycle;

    FlowMeter &flow;
    Cistern &cistern;

    Pump(int pwmChannel, int pwmPin, FlowMeter &flow, Cistern &cistern);

    // Implement to inherit from abstract class
    // ISubstateMachine (aka C++ "interface")
    void loop();
    bool machineDone();
    void resetMachine();

    void add_callback(callback act);

private:
    void setup();
    void setupPWM();
    void switchOn();
    void switchOff();
    void commonStateLogic();
    bool countTime(float durationSec);
    void printError();

    // Direct Func, no StateMachine run?
    // bool testPumpPerformance();

    callback setupToFs;
};

extern Pump pump1, pump2;

#endif // Pump_h