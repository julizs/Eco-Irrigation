#ifndef StatusDisplay_h
#define StatusDisplay_h

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <Pins.h>

#define MAX_DEVICES 2
// Pins Alloc see Pins.h
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//MD_Parola Display = MD_Parola(HARDWARE_TYPE, DATA, CLK, CS, MAX_DEVICES);

class StatusDisplay
{
public:
    StatusDisplay();
    void setupLEDMatrix();
    void displayPlantStatus();
};

extern MD_Parola Display;

#endif // StatusDisplay_h