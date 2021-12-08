#ifndef multiplexer.h
#define multiplexer.h
#include <pins.h>
#include <Arduino.h>

class Multiplexer
{
public:
    Multiplexer();
    static void setup();
    static int readChannel(byte channel);
};

#endif //multiplexer.h