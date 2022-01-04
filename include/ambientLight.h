#ifndef ambientLight_h
#define ambientLight_h
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
    TwoWire bus;
    int sensorId;

    // Two TSL2591 on different busses, since I2C Adress is SW and HW locked
    AmbientLight(int sensorId, TwoWire &bus);
    void setupTSL2591();
    TSL2591data measureLight();

private:
    void readTSL2591();
};

#endif //ambientLight_h