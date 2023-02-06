#ifndef Pump_h
#define Pump_h
#include <Cistern.h>
#include <FlowMeter.h>
#include <Irrigation.h>

struct Instruction; // Forward Decl

enum class PumpState
{
    INIT = 0,
    ON,
    DONE,
    ABORT
};

class Pump: public ISubStateMachine
{
    // Func Pointer to call Method from Main
    using callback = void (*)();

public:
    PumpState currentState, lastState;
    char const *stateNames[4] = {"PUMP_INIT", "PUMP_ON", "PUMP_DONE", "PUMP_ABORT"};
    char const *errors[6] = {"None", "ToF Setup failed.", "Not enough Water for Irrigation in Reservoir", "Irrigation cancelled by User."};
    
    bool isDone;
    uint8_t transCount, maxSelfTrans, errorCode;
    uint32_t stateBeginMillis, minStateDuration, measureIntervall, currentTime, lastTime;

    // Irrigation instruction
    Instruction *instr;

    // Pwm Control
    uint8_t resolution, pwmChannel;
    uint16_t frequency, dutyCycle;

    FlowMeter *flow;
    Cistern &cistern;

    Pump(Cistern &cistern);

    // Impl. to inherit from abstract class ISubstateMachine (aka C++ "interface")
    void loop();
    bool machineDone();
    void resetMachine();

    void add_callback(callback act);

private:
    Utilities utils;
    void setup();
    void setupPWM();
    void switchOn();
    void switchOff();
    void commonStateLogic();
    // bool countTime(float durationSec);
    void printError();

    callback setupToFs;
};

extern Pump pump1, pump2;

#endif // Pump_h