#ifndef climate_h
#define climate_h
#include <DHTesp.h>
#include <pins.h>

// scoped enumeration for same stateNames
enum class MeasureState
{
    MEASURE,
    IDLE,
    REMEASURE
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
    DHTdata data;
    int maxPollingRate;
    int measureAttemps;
    float temperature;
    float humidity;
    bool measurementsComplete;
    MeasureState currentState, lastState;
    unsigned long stateBeginMillis;
 
    Climate(int maxPollingRate, int measureAttempts); // Konstr 
    void setup();
    DHTdata loop();
    DHTdata measureClimateDHT();
    float measureHumidityDHT();
    float measureTemperatureDHT();
    bool validHumidity();
    bool validTemperature();
};

#endif // climate_h