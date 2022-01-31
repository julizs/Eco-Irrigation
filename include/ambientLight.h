#ifndef AmbientLight_h
#define AmbientLight_h
#include "Adafruit_TSL2591.h"
#include <SPI.h>
#include <main.h>

struct TSL2591data
{
    int visibleLight;
    int infraRed;
};

class AmbientLight
{
public:
    Adafruit_TSL2591 tsl;
    int sensorId;

    AmbientLight(int sensorId);
    void setupTSL2591(TwoWire &bus);
    TSL2591data measureLight();

private:
    void readTSL2591();
};

#endif // AmbientLight_h