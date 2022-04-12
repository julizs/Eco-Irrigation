#ifndef ButtonHandler_h
#define ButtonHandler_h

#include <WebServer.h>
#include <main.h>

class ButtonHandler
{
public:
    static WebServer webServer;
    // (Hardware Btn) Debouncing
    static uint16_t lastDebounceTime;               // Last time Output Pin was toggled
    static uint8_t debounceDelay;                   // Increase if Output flickers
    static uint8_t buttonState, lastButtonState;    // Initial State is Off

    static void startRestServer();
    static void handleHardwareButtons();
    static void handleWebButtons();
    
private:
    static void rootEndpoint();

};

#endif // ButtonHandler.h