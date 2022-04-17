#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <InfluxHelper.h>
#include <Pump.h>
#include <Action.h>

struct SolenoidMl { 
    uint8_t solenoidValve;
    uint16_t waterAmountMl;  
};

// Also used to write Report
struct instruction {
    uint8_t solenoidValve;
    uint16_t waterAmountMl;
    char plantName[];
    float pumpTime;
    Pump &pump;
};

class Irrigation
{
    public:
        static bool didEvaluate;
        static int waterLimit2h, waterLimit24h, pumpedWater2h, pumpedWater24h;
        static std::vector<SolenoidMl> vec2h, vec24h; // Vector of int[] not possible
        static FluxQueryResult cursor2h, cursor24h;

        static void decidePlants();
        static void writeInstruction(std::vector<instruction> &instruction);
        static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
        static FluxQueryResult recentIrrigations(uint8_t timePeriod);
        static bool writeVector(FluxQueryResult &cursor, std::vector<SolenoidMl> &vec);
};
#endif // Irrigation_h