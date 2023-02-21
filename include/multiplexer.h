#ifndef Multiplexer_h
#define Multiplexer_h

#include <pins.h>
#include <Arduino.h>

class Multiplexer
{
private:
    static int controlPins[4];
    static int muxChannel[16][4];

public:
    static void setupPins();
    static uint16_t readChannel(uint8_t channel);
};

#endif // Multiplexer.h