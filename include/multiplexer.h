#ifndef Multiplexer_h
#define Multiplexer_h

#include <pins.h>
#include <Arduino.h>

class Multiplexer
{
public:
    
    static void setup();
    static int readChannel(byte channel);
};

#endif // Multiplexer.h