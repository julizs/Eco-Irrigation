#ifndef fotoresistor_h
#define fotoresistor_h
#include "Arduino.h"

class Fotoresistor
{
public:
    Fotoresistor(int resistance, float voltageIn, uint8_t pinNum);
    uint8_t pin;
    int res;
    float vin;
    int measureLight();
};

#endif // fotoresistor_h