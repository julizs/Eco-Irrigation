#ifndef pump_h
#define pump_h
#include <main.h>
#include "Adafruit_VL53L0X.h"

enum class PumpState
{
    IDLE,
    ON,
    DONE
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

    //VL53L0X toF; // Polulu
    Adafruit_VL53L0X toF_1 = Adafruit_VL53L0X();
    unsigned short currWaterDist, minWaterDist, maxWaterDist; // aka minWaterLevel
    float mmToMlFactor; // depends on WaterTank
    bool ToF_ready;

    Pump(PumpModel& pumpModel); // Konstr.
    const PumpModel& getPumpModel() const;
    void setPumpModel(const Pump::PumpModel& pM);
    void setup();
    void loop();
    bool setupToF();
    int readToF();
    int readToF_cont();

private:
    void switchOn();
    void switchOff(); 
    void setupPWM();
    bool checkWaterLevel();
    bool checkPumpPerformance(unsigned short);
};

#endif // pump_h