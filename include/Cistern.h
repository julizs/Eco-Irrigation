#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include <main.h>

/*
Each pump has 1 associated ToF Sensor, Water Temp Sensor, Water PH Sensor
*/
class Cistern
{
    friend class Pump;
    public:
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF;
        uint8_t* solenoidValves; // relaisChannels
        uint8_t toF_address, sampleSize;
        int cisternHeight, currWaterDist, minWaterDist, maxWaterDist;
        float mmToMl;
        bool toF_ready;
        unsigned long stateBeginMillis;

        Cistern(uint8_t toF_address, uint8_t solenoidValves[], int cisternHeight, float mmToMl);
        bool setupToF();
        int updateWaterLevel();         

    private:
        float evaluateToF();
        void readToF(int distances[]);
        void readToF_cont(int distances[]);  
        void updateIrrigations(uint8_t relaisChannel);
        void driveSolenoid(uint8_t relaisChannel, uint8_t state);
        int calcMl(float waterLevel);
        bool validWaterLevel();
        void shutToF();
};

extern Cistern cistern1, cistern2;

#endif // Cistern_h