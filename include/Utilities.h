#ifndef Utilities_h
#define Utilities_h

#include <main.h>

class Utilities
{
    public:
        static void scanI2CBus(TwoWire *wire);
        static DynamicJsonDocument readFlash(int address);
        static void writeFlash(DynamicJsonDocument &doc);
};

#endif // Utilities_h