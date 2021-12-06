#ifndef climate_h
#define climate_h
#include <DHTesp.h>
#include <pins.h>

// scoped enumeration for same stateNames
enum class MeasureState
{
    INIT,
    IDLE,
    MEASURING
};

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
    float temperature;
    float humidity;
    MeasureState currentState, lastState;
    unsigned long stateBeginMillis;
 
    Climate(int maxPollingRate, int measureAttempts); // Konstr 
    void setup();
    DHTdata loop();
    DHTdata measureClimateDHT();
    float measureHumidityDHT();
    float measureTemperatureDHT();
};

#endif // climate_h