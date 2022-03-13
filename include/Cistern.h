#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include <VL53L0X.h>
#include <main.h>

/*
Has 1 ToF Sensor, Water Temp Sensor, Water PH Sensor, 1 Pump?
*/
class Cistern
{
    public:
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF = Adafruit_VL53L0X();
        unsigned short toF_address;
        unsigned short currWaterDist, minWaterDist, maxWaterDist; // aka minWaterLevel
        bool toF_ready;

        // Cistern();
        // Cistern(int toF_address);
        bool setupToF(int toF_address);

        float evaluateToF();
        void readToF(Adafruit_VL53L0X toF, int distances[]);
        float readToF_cont();
        void updateWaterLevel();
        bool checkWaterLevel();

    private:
        
};

#endif // Cistern_h