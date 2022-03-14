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
        unsigned short toF_address, sampleSize;
        unsigned short currWaterDist, minWaterDist, maxWaterDist; // aka minWaterLevel
        bool toF_ready;

        // Cistern();
        Cistern(int toF_address);
        bool setupToF();
        void shutToF();

        float evaluateToF();
        void readToF(int distances[]);
        float readToF_cont();
        void updateWaterLevel();
        bool checkWaterLevel();

    private:
        
};

#endif // Cistern_h