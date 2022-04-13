#ifndef ButtonHandler_h
#define ButtonHandler_h

#include <main.h>

class ButtonHandler
{
public:
    // (Hardware Btn) Debouncing
    static uint16_t lastDebounceTime;               // Last time Output Pin was toggled
    static uint8_t debounceDelay;                   // Increase if Output flickers
    static uint8_t buttonState, lastButtonState;    // Initial State is Off

    static void handleHardwareButtons();
    
private:

};

#endif // ButtonHandler.h