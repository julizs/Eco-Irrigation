#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <InfluxHelper.h>
#include <Pump.h>
#include <Action.h>

class Irrigation
{
    public:
        static bool didEvaluate;

        static void decideIrrigation();
        static void decidePlants(uint8_t solenoidValve, int recentWater, DynamicJsonDocument &plants, DynamicJsonDocument &plantNeeds);
        static void getIrrigationInfo(uint8_t solenoidValve, int irrigationAmount);
        static void validSolenoids(bool validSolenoids[]);
        static bool validSolenoid(uint8_t solenoidValve);
        static int recentIrrigations(uint8_t timePeriod, uint8_t solenoidValve);
};

#endif // Irrigation_h