#ifndef Utilities_h
#define Utilities_h

#include <main.h>
#include <Irrigation.h>

class Utilities
{
    public:
        static void scanI2CBus(TwoWire *wire);
        static DynamicJsonDocument readDoc(int address, int size);
        static void writeDoc(int address, DynamicJsonDocument &doc, int size);
        static bool provideData();
};

#endif // Utilities_h