#ifndef fotoresistor_h
#define fotoresistor_h
#include "main.h"

class Fotoresistor
{
public:
    Fotoresistor();
    Fotoresistor(int resistance, float voltageIn, byte sampleSize, byte multiPin);
    byte multiPin;
    byte sampleSize;
    int res;
    float vin;
    
    int measureLux(float rawVal);
    int measureLight();
    float measureLightSmoothed();
};

#endif // fotoresistor_h