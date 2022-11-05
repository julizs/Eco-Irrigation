#ifndef Multiplexer_h
#define Multiplexer_h

#include <pins.h>
#include <Arduino.h>

class Multiplexer
{
public:
    
    static void setupPins();
    static int readChannel(uint8_t channel);
};

#endif // Multiplexer.h