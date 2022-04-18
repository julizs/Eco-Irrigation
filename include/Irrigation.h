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
struct Instruction {
    char plantName[32];
    float pumpTime;
    uint8_t solenoidValve;
    uint16_t waterAmount;
    Pump* pump;
};

class Irrigation
{
    public:
        static bool didEvaluate;
        static const uint16_t waterLimit2h = 500, waterLimit24h = 1000;
        static const float pumpTimeLimit = 10.0f;
        // static FluxQueryResult ml2h, ml24h;

        static void decidePlants();
        static void writeInstructions(std::vector<Instruction> &instructions);
        static void printInstructions(std::vector<Instruction> &instructions);
        static FluxQueryResult recentIrrigations(uint8_t timePeriod);
        static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);  
        static bool prepData();
        static bool sendReport();
};
#endif // Irrigation_h