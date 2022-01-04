#ifndef ambientLight_h
#define ambientLight_h
#include "Adafruit_TSL2591.h"
#include <SPI.h>
#include <main.h>

class AmbientLight
{
public:
    Adafruit_TSL2591 tsl;
    TwoWire bus;
    int sensorId;

    // Two TSL2591 on different busses, since I2C Adress is SW and HW locked
    AmbientLight(int sensorId, TwoWire &bus);
    void setupTSL2591();
    void readTSL2591();

private:

};

#endif //ambientLight_h