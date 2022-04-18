#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <Pump.h>

class Pump; // Forward Decl

/*
Infos needed for Irrigation Process
Also used to write Report
*/
struct instruction {
    char plantName[];
    float pumpTime;
    uint8_t solenoidValve;
    uint16_t waterAmountMl; 
    Pump &pump;
};

class Irrigation
{
    public:
        static bool didEvaluate;
        static int waterLimit2h, waterLimit24h, pumpedWater2h, pumpedWater24h;
        static FluxQueryResult cursor2h, cursor24h;

        static void decidePlants();
        static void writeInstruction(std::vector<instruction> &instruction);
        static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
        static FluxQueryResult recentIrrigations(uint8_t timePeriod);
        static bool prepData();
};
#endif // Irrigation_h