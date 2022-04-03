#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include <main.h>

/*
Each pump has 1 associated ToF Sensor, Water Temp Sensor, Water PH Sensor
*/
class Cistern
{
    public:
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF = Adafruit_VL53L0X();
        uint8_t* solenoidValves; // relaisChannels
        uint8_t toF_address, sampleSize;
        int cisternHeight, currWaterDist, minWaterDist, maxWaterDist;
        float mmToMl;
        bool toF_ready;

        // Cistern();
        Cistern(uint8_t toF_address, uint8_t solenoidValves[], int cisternHeight, float mmToMl);
        void setupToF();
        void shutToF();

        float evaluateToF();
        void readToF(int distances[]);
        void readToF_cont(int distances[]);
        int updateWaterLevel();
        void updateIrrigations(uint8_t relaisChannel);
        bool validWaterLevel();
        int calcMl(float waterLevel);
        void driveSolenoid(uint8_t relaisChannel, uint8_t state);

    private:
        
};

extern Cistern cistern1, cistern2;

#endif // Cistern_h