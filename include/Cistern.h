#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include "FlowMeter.h"
#include <main.h>

/*
Each pump has 1 associated ToF Sensor, Water Temp Sensor, Water PH Sensor
solenoidValves: Which Relais Channels are associated with this Cistern
Since 1 Esp constrols 1-2 pumps, 1 toF, 2-4 Sols/relaisChannels its unnecessary to have multiple toFs etc.

*/
class Cistern
{
    friend class Pump;
    public:
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF;
        FlowMeter &meter;
        uint8_t* solenoidValves;
        uint8_t toF_address, sampleSize;
        uint16_t pumpedWater;
        int currWaterLevel, currWaterDist, minValidWaterDist, maxValidWaterDist;
        int maxPossibleDist; //  
        bool toF_ready;
        unsigned long stateBeginMillis;

        Cistern(uint8_t toF_address, FlowMeter &meter);
        bool setupToF();
        int updateWaterLevel();
        bool waterManagement();         

    private:
        float evaluateToF();
        void readToF(int distances[]);
        void readToF_cont(int distances[]);  
        void updateIrrigationData(uint8_t relaisChannel, int pumpedWater);
        void updateEnvironmentData(int newWaterLevel, int availableWater);
        void driveSolenoid(uint8_t relaisChannel, uint8_t state);
        uint16_t calcMl(int waterLevel);
        bool validWaterLevel();
        void shutToF();
};

extern Cistern cistern1, cistern2;

#endif // Cistern_h