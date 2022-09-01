#ifndef Utilities_h
#define Utilities_h

#include <ArduinoJson.h>
#include <Wire.h>

#include <EEPROM.h>
#include <StreamUtils.h>
#include <InfluxHelper.h>

#include <main.h>

struct WaterPerSolenoid { 
    int solenoidValve; // -1 to 4
    uint8_t timePeriod; // h
    uint16_t waterAmount; // ml
};

class Utilities
{
    public:
        static uint32_t stateBeginMillis;

        static uint8_t scanI2CBus(TwoWire *wire);
        static DynamicJsonDocument readDoc(int address, int size);
        static void writeDoc(int address, DynamicJsonDocument &doc, int size);
        static bool countTime(long begin, uint8_t duration);
        static bool cursorToVec(FluxQueryResult &cursor, std::vector<WaterPerSolenoid> &vec, uint8_t timePeriod);
        static void printCursor(FluxQueryResult &cursor);
        static void printSolenoids(std::vector<WaterPerSolenoid> &solenoids);
        static bool containsDestination(String dest);
        static void printDestinations();
};

#endif // Utilities_h