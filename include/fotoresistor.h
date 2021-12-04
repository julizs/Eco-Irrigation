#ifndef fotoresistor_h
#define fotoresistor_h
#include "Arduino.h"

class Fotoresistor
{
public:
    Fotoresistor(int resistance, float voltageIn, byte sampleSize);
    byte pin;
    byte sampleSize;
    int res;
    float vin;
    int measureLux(float rawVal);
    int measureLight();
    float measureLightSmoothed();
};

#endif // fotoresistor_h