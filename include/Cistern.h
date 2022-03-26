#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include <VL53L0X.h>
#include <main.h>

/*
Each pump has 1 associated ToF Sensor, Water Temp Sensor, Water PH Sensor
*/
class Cistern
{
    public:
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF = Adafruit_VL53L0X();
        unsigned short toF_address, sampleSize, cisternHeight;
        unsigned short currWaterDist, minWaterDist, maxWaterDist; // aka minWaterLevel
        float mmToMl;
        bool toF_ready;

        // Cistern();
        Cistern(unsigned short toF_address, unsigned short cisternHeight, float mmToMl);
        void setupToF();
        void shutToF();

        float evaluateToF();
        void readToF(int distances[]);
        void readToF_cont(int distances[]);
        int updateWaterLevel();
        void updateIrrigations();
        bool validWaterLevel();
        int calcMl(float waterLevel);

    private:
        
};

#endif // Cistern_h