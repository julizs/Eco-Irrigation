#ifndef pump_h
#define pump_h
#include <Arduino.h>
#include <Wire.h>
#include <pins.h>
#include <VL53L0X.h>

enum class PumpState
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
    const char *stateNames[2] = {"PUMP_IDLE", "PUMP_ON"};
    unsigned long stateBeginMillis, minStateDuration;
    bool pumpSignal, cycleDone;

    VL53L0X toF; // see Library Doc
    unsigned short maxWaterDistance; // min allowed WaterLevel
    unsigned short waterDistance; // measured WaterLevel
    unsigned short distanceDelta;
    float mmToMlFactor; // depends on WaterTank
    float totalPumped, lastPumped, bestPumped;

    Pump(PumpModel& pumpModel); // Konstr.
    const PumpModel& getPumpModel() const;
    void setPumpModel(const Pump::PumpModel& pM);
    void setup();
    void loop();
    float getWaterLevel();

private:
    void switchOn();
    void switchOff();
    void setupToFSensor();
    bool checkMinWaterDistance();
    bool checkPumpPerformance(unsigned short);
    float distanceDeltaToMilliliters(unsigned short);
};

#endif // pump_h