#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <InfluxHelper.h>
#include <Pump.h>

class Irrigation
{
    static int recentIrrigations[];
    public:
        static void decideIrrigation();
        static void getIrrigationInfo(uint8_t solenoidValve, int irrigationAmount);
        static bool validSolenoid(FluxQueryResult &cursor, uint8_t relaisChannel);
};

#endif // Irrigation_h