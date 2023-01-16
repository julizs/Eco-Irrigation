#ifndef Cistern_h
#define Cistern_h

#include "Adafruit_VL53L0X.h"
#include "FlowMeter.h"
// #include <main.h>

enum class LiquidType
{
    WATER = 0,
    FERTILIZER
};

/*
Each Cistern has 1-2 Pumps, 1 toF ?
Each Pump assotiated to exactly 1 Cistern
*/
class Cistern
{
    // friend class Pump;

    public:
        LiquidType contents;
        //VL53L0X toF; // Polulu Library
        Adafruit_VL53L0X toF;
        // uint8_t* solenoidValves;
        uint8_t toF_address, sampleSize;
        // uint16_t pumpedLiquid;
        uint16_t currLiquidLevel, currLiquidDist, currLiquidAmount,
        minValidLiquidLevel, maxValidLiquidLevel, maxMeasurableDist;

        Cistern(uint8_t toF_address);
        void setupToF();
        bool toF_ready();
   
        uint16_t getLiquidLevel(); // mm
        uint16_t getLiquidAmount(); // ml
        bool validLiquidLevel(int allocatedWater);

        void updateLiquidPumped();
        void updateLiquidAmount();
        void driveSolenoid(uint8_t relaisChannel, uint8_t state);   

    private: 
        void readToF(int distances[]);
        void readToF_cont(int distances[]);
        float evaluateToF();
        uint16_t calcMl(int waterLevel); // as per Shape of Cistern

        void shutToF();
};

extern Cistern cistern1, cistern2;

#endif // Cistern_h