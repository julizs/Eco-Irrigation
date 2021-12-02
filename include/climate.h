#ifndef climate_h
#define climate_h
#include <DHTesp.h>

#define DHTpin 0
//#define AHTpin 0

struct DHTdata
{
    float temperature;
    float humidity;
};

class Climate
{
    public:
    DHTesp dht;
    int maxPollingRate;
    int measureAttemps;
 
    Climate(int maxPollingRate, int measureAttempts); // Konstr 
    void InitialiseDHT();
    DHTdata MeasureDHT();
    float MeasureHumidityDHT();
    float MeasureTemperatureDHT();
};

#endif // climate_h