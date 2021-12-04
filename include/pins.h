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
#define A 14
#define B 12
#define C 13
#define analogPin A0
#endif

#endif //pins_h