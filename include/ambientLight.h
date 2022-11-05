#ifndef AmbientLight_h
#define AmbientLight_h
#include "Adafruit_TSL2591.h"
#include <SPI.h>
#include <main.h>

struct LightData
{
    uint32_t fullSpectrum;
    uint16_t visibleLight, infraRed;
    float illuminance; // Lux
};

class AmbientLight
{
public:
    Adafruit_TSL2591 tsl;
    int sensorId;
    LightData data;

    AmbientLight(int sensorId);
    void setup(TwoWire &bus);
    void printParameters();
    bool isReady();
    LightData measure();

private:
    void printMeasurement(LightData &measurement);
};

#endif // AmbientLight_h