#ifndef pins_h
#define pins_h

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#define pumpPin 5
// Multiplexer
#define Aa 14
#define Be 12
#define Ce 13
#define analogPin A0
#endif

// Egal ob Esp32 oder Esp8266
#define C0 (byte)0
#define C1 (byte)1

#endif //pins_h