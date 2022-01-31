#ifndef AmbientClimate_h
#define AmbientClimate_h
#include <DHTesp.h>
#include <main.h>

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

class AmbientClimate
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
 
    AmbientClimate(int maxPollingRate, int measureAttempts); // Konstr 
    void setup();
    void loop();
    DHTdata measureClimateDHT();
    float measureHumidityDHT();
    float measureTemperatureDHT();
    bool validHumidity();
    bool validTemperature();
};

#endif // AmbientClimate_h