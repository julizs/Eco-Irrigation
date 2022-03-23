#ifndef Pump_h
#define Pump_h
#include <main.h>
#include <Cistern.h>
#include "Adafruit_INA219.h"

enum class PumpState
{
    IDLE,
    ON,
    DONE
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

class Pump
{
public:

    PumpState currentState, lastState;
    const char *stateNames[3] = {"PUMP_IDLE", "PUMP_ON", "PUMP_DONE"};
    unsigned long stateBeginMillis, minStateDuration, maxStateDuration;

    DynamicJsonDocument pumpModel;
    int pwmPin, pwmChannel, frequency, resolution, dutyCycle;

    Cistern cistern;
    Adafruit_INA219 ina219;
    float voltage_V, shuntVoltage_mV, busVoltage_V;
    float current_mA, power_mW;

    Pump(int pwmChannel, int pwmPin, Cistern& cistern);
    void setup();
    void loop();
    void prepareIrrigation(const char* plantGroup, int irrigationAmount);
    void doIrrigation(const char* pumpName, int irrigationAmount); 

    bool setupIna();
    INAdata readIna();
    void writeIna();

private:
    void switchOn();
    void switchOff(); 
    void setupPWM();
    bool checkPumpPerformance(unsigned short);
};

#endif // Pump_h