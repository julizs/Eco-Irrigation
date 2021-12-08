#ifndef pins_h
#define pins_h

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#define wakeUpPin 16 //D0
#define pumpButtonPin 5 //D1
#define relaisPin 4 //D2 
#define laserDistPin 0 //D3 
#define DHTpin 2 //D4
//#define AHTpin 0

// Multiplexer
#define S0 14
#define S1 12
#define S2 13
#define S3 15
#define analogPin A0

// L298N
#define enA 15
#define in1 3
#define in2 1
#endif

// Egal ob Esp32 oder Esp8266
#define C0 0
#define C15 15

#endif //pins_h