#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>
#include <Utilities.h>
#include <InfluxHelper.h>

class Irrigation
{
    static int recentIrrigations[];
    public:
        static void decideIrrigation();
        static void getIrrigationInfo(const char* plantName, int irrigationAmount);
        static void convert(FluxQueryResult &cursor, int result[]);
        static bool validSolenoid(FluxQueryResult &cursor, uint8_t relaisChannel);
};

#endif // Irrigation_h