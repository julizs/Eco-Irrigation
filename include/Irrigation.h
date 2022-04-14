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

class Irrigation
{
    public:
        static bool didEvaluate;
        static int waterLimit2h, waterLimit24h, pumpedWater2h, pumpedWater24h;
        static std::vector<SolenoidMl> vec2h, vec24h; // Vector of int[] not possible
        static FluxQueryResult cursor2h, cursor24h;

        static void decideIrrigation();
        static void decidePlants(uint8_t solenoidValve, DynamicJsonDocument &plants, DynamicJsonDocument &plantNeeds);
        static void getIrrigationInfo(uint8_t solenoidValve, int irrigationAmount);
        static bool validSolenoid(uint8_t solenoidValve, uint16_t waterLimit);
        static FluxQueryResult recentIrrigations(uint8_t timePeriod);
        static bool writeVector(FluxQueryResult &cursor, std::vector<SolenoidMl> &vec);
};
#endif // Irrigation_h