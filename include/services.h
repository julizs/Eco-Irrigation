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

//#define WIFI_SSID "FRITZ!Box 7430 ED"
//#define WIFI_PASSWORD "49391909776212256241"
#define WIFI_SSID "FRITZ!Box 6490 Cable"
#define WIFI_PASSWORD "04623152464666844388"

class Services
{
    public:
    Services();
    void setupWifi();
    void setupWifiMulti();
    void doGetRequest();
    DynamicJsonDocument doJSONGetRequest(char url[]);
    bool getWifiStatus();
    bool getWifiMultiStatus();
};

#endif // Services_h