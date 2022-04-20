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
struct Instruction
{
    char reason[32], pumpModel[32];
    Pump *pump;
    float pumpTime;  
    uint8_t errorCode, solenoidValve;
    uint16_t waterAmount;

    bool operator==(const Instruction &instr) const
    {
        return (solenoidValve == instr.solenoidValve);
    }
};

class Irrigation
{
public:
    static bool didEvaluate;
    static const uint16_t waterLimit2h = 500, waterLimit24h = 1000;
    static constexpr float pumpTimeLimit = 10.0f;  
    static std::vector<Instruction> instructions;
    // static FluxQueryResult ml2h, ml24h;

    static void decidePlants();
    static void writeInstructions(std::vector<Instruction> &instructions);
    static void reduceInstructions(std::vector<Instruction> &instructions);
    static void sortInstructions(std::vector<Instruction> &instructions);
    static void printInstructions();
    static bool reportInstructions();
    static bool compareBySolenoid(const Instruction &a, const Instruction &b);
    static FluxQueryResult recentIrrigations(uint8_t timePeriod);
    static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
    static bool cursorToVec();
};
#endif // Irrigation_h