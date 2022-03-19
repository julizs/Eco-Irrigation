#ifndef ButtonHandler_h
#define ButtonHandler_h

#include <WebServer.h>
#include <Pins.h>

class ButtonHandler
{
public:
    static WebServer webServer;

    static void startRestServer();
    static void handleHardwareButtons();
    static void root();
};

#endif // ButtonHandler.h