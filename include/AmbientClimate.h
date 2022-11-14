#ifndef AmbientClimate_h
#define AmbientClimate_h
#include <DHTesp.h>
#include <main.h>

enum class DhtState
{
    IDLE,
    MEASURE,
    DONE
};

struct DhtData
{
    float temperature;
    float humidity;
};

class AmbientClimate
{
    public:
    DhtData data;
    uint8_t pinNum;  
    DhtState currentState, lastState;
 
    // DHT 11/22 different pollingRates
    AmbientClimate(float maxPollingRate, uint8_t pinNum);
    void setup();
    void loop();
    DhtData measureClimate();

    private:
    DHTesp dht;
    unsigned long stateBeginMillis;
    float maxPollingRate, minStateTime; 
    float measureHumidity();
    float measureTemperature();
    bool validHumidity(float humidity);
    bool validTemperature(float temperature);
    void sensorInfo();
    void writePoint();
};

#endif // AmbientClimate_h