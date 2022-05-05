#ifndef Utilities_h
#define Utilities_h

#include <ArduinoJson.h>
#include <Wire.h>

#include <EEPROM.h>
#include <StreamUtils.h>
#include <InfluxHelper.h>

struct MlPerSolenoid { 
    uint8_t solenoidValve;
    uint16_t waterAmount;
};

class Utilities
{
    public:
        static std::vector<MlPerSolenoid> ml2h, ml24h;

        static uint8_t scanI2CBus(TwoWire *wire);
        static DynamicJsonDocument readDoc(int address, int size);
        static void writeDoc(int address, DynamicJsonDocument &doc, int size);
        static bool countTime(long begin, uint8_t duration);
        static bool writeVector(FluxQueryResult &cursor, std::vector<MlPerSolenoid> &vec);
        static void printMlPerSolenoid(std::vector<MlPerSolenoid> &solenoids);
};

#endif // Utilities_h