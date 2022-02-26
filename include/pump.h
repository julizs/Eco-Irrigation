#ifndef Pump_h
#define Pump_h
#include <main.h>
#include "Adafruit_VL53L0X.h"
#include "Adafruit_INA219.h"
#include <VL53L0X.h>

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
    const char *stateNames[3] = {"PUMP_IDLE", "PUMP_ON", "PUMP_DONE"};
    unsigned long stateBeginMillis, minStateDuration;

    //int pwmFrequency, pwmChannel, pwmResolution;
    const int pwmChannel1 = 0;
    const int pwmChannel2 = 1;
    const int freq = 30000;
    const int res = 8;
    int dutyCycle = 200;

    //VL53L0X toF; // Polulu
    //VL53L0X toF_2;
    Adafruit_VL53L0X toF_1 = Adafruit_VL53L0X();
    Adafruit_VL53L0X toF_2 = Adafruit_VL53L0X();
    unsigned short currWaterDist, minWaterDist, maxWaterDist; // aka minWaterLevel
    bool toF_1_ready, toF_2_ready;

    Adafruit_INA219 ina219;
    float voltage_V, shuntVoltage_mV, busVoltage_V;
    float current_mA, power_mW;

    Pump(PumpModel& pumpModel); // Konstr.
    const PumpModel& getPumpModel() const;
    void setPumpModel(const Pump::PumpModel& pM);
    void prepareIrrigation(const char* plantGroup, int irrigationAmount);
    void doIrrigation(const char* pumpName, int irrigationAmount);
    void setup();
    void loop();
    bool setupToF_1();
    bool setupToF_2();

    bool setupIna_1();
    INAdata readIna_1();

    float readToF(Adafruit_VL53L0X toF);
    float readToF_cont();

private:
    void switchOn();
    void switchOff(); 
    void setupPWM();
    bool checkWaterLevel();
    bool checkPumpPerformance(unsigned short);
};

#endif // Pump_h