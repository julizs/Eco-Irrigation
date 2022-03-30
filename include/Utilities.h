#ifndef Utilities_h
#define Utilities_h

#include <main.h>

class Utilities
{
    public:
        static void scanI2CBus(TwoWire *wire);
        static DynamicJsonDocument readDoc(int address);
        static void writeDoc(int address, DynamicJsonDocument &doc);
};

#endif // Utilities_h