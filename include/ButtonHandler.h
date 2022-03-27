#ifndef ButtonHandler_h
#define ButtonHandler_h

#include <WebServer.h>
#include <main.h>

class ButtonHandler
{
public:
    static WebServer webServer;
    // (Hardware Btn) Debouncing
    static unsigned long lastDebounceTime;      // Last time Output Pin was toggled
    static unsigned short debounceDelay;         // Increase if Output flickers
    static int buttonState, lastButtonState;    // Initial State is Off

    static void startRestServer();
    static void handleHardwareButtons();
    static void handleWebButtons();
    static void rootEndpoint();
};

#endif // ButtonHandler.h