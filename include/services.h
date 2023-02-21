#ifndef Services_h
#define Services_h

#if defined(ESP32)
// #include <WiFiMulti.h>
// extern WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
extern ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif
// #include <main.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <lwip/dns.h>
#include <Utilities.h>
#include <Irrigation.h>
#include <Settings.h>

#define WIFI_SSID "FRITZ!Box 7430 ED"
#define WIFI_PASSWORD "49391909776212256241"

#define AP_SSID "esp32-v6"

class Services
{
    public:
        static HTTPClient http;
        static WiFiClient client;
        // static WiFiMulti wifiMulti;   
        // static WiFiServer wifiServer;
        // static WebServer webServer;
        // static ip_addr_t ip6_dns;

        static void setupWifi();
        static bool wifiConnected();
        // static void setupWifiMulti();
        // static bool wifiMultiConnected();
        // static void wifiEventHandler(WiFiEvent_t event);

        static String doGetRequest(char const url[]);
        static void doJSONPostRequest(char const url[], String &payload);
        static void doJSONGetRequest(char const url[], DynamicJsonDocument &doc);
        static void startRestServer();
        static bool readSettings();
        // static void handleWebButtons();

    private:
        static void rootEndpoint();
};  

#endif // Services_h