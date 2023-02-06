#ifndef Irrigation_h
#define Irrigation_h

// #include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <Pump.h>
#include <Settings.h>

using std::vector;

// class Pump; // Forward Decl

struct Instruction
{
    char reason[32], pumpModel[32];
    float pumpTime;
    int8_t errorCode, solenoidValve, pwmChannel;
    uint16_t allocatedWater, distributedWater;

    bool operator==(const Instruction &instr) const
    {
        return (solenoidValve == instr.solenoidValve);
    }
};

class Irrigation
{
    friend class Evaluation;

public:
    // static constexpr float pumpTimeLimit = 10.0f;
    // static FluxQueryResult waterPerSol;

    static vector<Instruction> instructions;
    const vector<Instruction> & getInstructions() const;
       
    static String errors[7];
    // static char const *errors[5];
    // static std::string errors[5];
    /* static geht nicht:
    char const *errors[5] = {"None", "Did not find Plant", "Solenoid invalid (Waterlimit)",
                             "Pump has no Solenoids assigned.", "Pump has no valid Solenoids (Waterlimit)"};*/                    

    static void createInstructions(JsonArray &actions, std::vector<Instruction> &instructions);
    static void writeInstructions();

    static bool reportInstruction(Instruction &instruction);
    static void clearInstructions();
    
    // Water Distr per Solenoid per Time Period
    static bool updateWaterDistribution();
    const vector<WaterPerSolenoid> & getWaterDistr() const;
    
    static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit, u16_t allocatedWater, uint8_t timePeriod);

    static void printError(uint8_t errorCode);

private:
    // static vector<Instruction> instructions;
    static vector<WaterPerSolenoid> waterDistribution;

    static bool evaluatePlants();

    static int8_t solenoidByPlant(Instruction &instr, DynamicJsonDocument &plants);
    static int8_t solenoidByPump(Instruction &instr, JsonObject &pumpModel);
    static JsonObject pumpModelBySolenoid(DynamicJsonDocument &pumps, uint8_t solenoidValve);
    static JsonObject pumpModelByName(DynamicJsonDocument &pumps, const char pumpName[]);
    static void setPumpingParameters(Instruction &instr, JsonObject &pumpModel);

    static void reduceInstructions(std::vector<Instruction> &instructions);
    static void sortInstructions(std::vector<Instruction> &instructions);
    static bool compareBySolenoid(const Instruction &i1, const Instruction &i2);

    // static bool reportInstructions(std::vector<Instruction> &instructions);
    // static bool reportToMongo(std::vector<Instruction> &instructions);

    static FluxQueryResult querySolenoids(uint8_t timePeriod);
    static uint8_t queryMoisture(uint8_t threshold, uint8_t timePeriod);
    static uint16_t waterPerSolenoid(uint8_t solenoidValve, uint8_t timePeriod);

    static void printInstructions(std::vector<Instruction> &instructions);
};
#endif // Irrigation_h