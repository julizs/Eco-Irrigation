#include "StatusDisplay.h"

MD_Parola Display = MD_Parola(HARDWARE_TYPE, DATA, CLK, CS, MAX_DEVICES);

StatusDisplay::StatusDisplay()
{
    //setupLEDMatrix();
}

void StatusDisplay::setupLEDMatrix()
{
  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();
}

void StatusDisplay::displayPlantStatus()
{
  Display.setTextAlignment(PA_CENTER);
  Display.print("Test");
}