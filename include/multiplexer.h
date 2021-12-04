#ifndef multiplexer.h
#define multiplexer.h
#include <pins.h>
#include <Arduino.h>

class Multiplexer
{
public:
    Multiplexer();
    void setup();
    int measureAnalog(byte pin);
};

#endif //multiplexer.h