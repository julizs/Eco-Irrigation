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
    int8_t errorCode, solenoidValve;
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
    static uint8_t solenoidByPlant(Instruction &instr, DynamicJsonDocument &plants);
    static uint8_t solenoidByPump(Instruction &instr, JsonObject &pumpModel);
    static JsonObject pumpModelBySolenoid(DynamicJsonDocument &pumps, uint8_t solenoidValve);
    static JsonObject pumpModelByName(DynamicJsonDocument &pumps, const char pumpName[]);
    static void setPumpTime(Instruction &instr, JsonObject &pumpModel);

    static void reduceInstructions(std::vector<Instruction> &instructions);
    static void sortInstructions(std::vector<Instruction> &instructions);
    static void printInstructions(std::vector<Instruction> &instructions);
    static bool reportInstructions(std::vector<Instruction> &instructions);
    static bool reportToMongo(std::vector<Instruction> &instructions);
    static void clearInstructions();
    static bool compareBySolenoid(const Instruction &a, const Instruction &b);
    static FluxQueryResult recentIrrigations(uint8_t timePeriod);
    static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
    static bool requestData();
    
};
#endif // Irrigation_h