#ifndef Services_h
#define Services_h

#if defined(ESP32)
#include <WiFiMulti.h>
extern WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
extern ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
// #include <ESPAsyncWebServer.h>
#include <main.h>
#include <WebServer.h>

// #define WIFI_SSID "FRITZ!Box 7430 ED"
// #define WIFI_PASSWORD "49391909776212256241"
#define WIFI_SSID "FRITZ!Box 6490 Cable"
#define WIFI_PASSWORD "04623152464666844388"

class Services
{
    public:
        static HTTPClient http;
        static WiFiClient client;
        static WiFiMulti wifiMulti;
        static WiFiServer wifiServer;
        static WebServer webServer;

        static void setupWifi();
        static void setupWifiMulti();
        static void doGetRequest(char const url[]);
        static void doPostRequest(char const url[]);
        static DynamicJsonDocument doJSONGetRequest(char const url[]);
        static bool getWifiMultiStatus();
        static void startRestServer();
        // static void handleWebButtons();
        static bool countTime(long begin, uint8_t duration);

    private:
        static void rootEndpoint();
};  

#endif // Services_h