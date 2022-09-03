#ifndef AmbientClimate_h
#define AmbientClimate_h
#include <DHTesp.h>
#include <main.h>

// https://stackoverflow.com/questions/4337193/how-to-set-enum-to-null
enum class MeasureState
{
    IDLE,
    MEASURE,
    DONE
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
    uint8_t pinNum;
    float maxPollingRate, minStateTime;
    float temperature, humidity;
    unsigned long stateBeginMillis;
    MeasureState currentState, lastState;
 
    AmbientClimate(float maxPollingRate, uint8_t pinNum);
    void setup();
    void loop();
    DHTdata measureClimateDHT();

    // private:
    float measureHumidityDHT();
    float measureTemperatureDHT();
    bool validHumidity();
    bool validTemperature();
    void printInfo();
    void writePoint();
};

#endif // AmbientClimate_h