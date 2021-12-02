#ifndef fotoresistor_h
#define fotoresistor_h
#include "Arduino.h"

class Fotoresistor
{
public:
    Fotoresistor(int resistance, float voltageIn, uint8_t pinNum);
    uint8_t pin;
    uint8_t measureAttempts;
    int res;
    float vin;
    int measureLux(float rawVal);
    int measureLight();
    float measureLightSmoothed();
};

#endif // fotoresistor_h