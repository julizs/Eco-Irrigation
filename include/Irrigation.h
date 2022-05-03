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
    // allocated and actual distributed Water (measured by waterFlow Sensor)
    uint16_t allocatedWater, distributedWater;


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
    static std::vector<Instruction> irrInstructions, pumpInstructions;
    // static FluxQueryResult ml2h, ml24h;

    static void decidePlants();
    static void createInstructions(JsonArray &actions, std::vector<Instruction> &instructions);
    static void writeInstructions();
    static bool pickSolenoidValve(Instruction &instr, JsonObject pumpModel);
    static uint8_t getSolenoidByPlant(Instruction &instr, DynamicJsonDocument &plants);
    static bool handlePumpActions(DynamicJsonDocument &pumps);
    static JsonObject findPumpModel(DynamicJsonDocument &pumps, uint8_t solenoidValve, const char pumpName[]);
    static void setPumpDetails(Instruction &instr, JsonObject pumpModel);

    static void reduceInstructions(std::vector<Instruction> &instructions);
    static void sortInstructions(std::vector<Instruction> &instructions);
    static void printInstructions();
    static bool reportInstructions();
    static bool compareBySolenoid(const Instruction &a, const Instruction &b);
    static FluxQueryResult recentIrrigations(uint8_t timePeriod);
    static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
    static bool requestData();
    
};
#endif // Irrigation_h