#ifndef pins_h
#define pins_h

#if defined(ESP32)
//#include <WiFiMulti.h>
//WiFiMulti wifiMulti;
#define DEVICE "ESP32"

#define button1Pin 15
#define button2Pin 4
//#define button3Pin 19
#define pumpPWM_Pin_1 23
#define pumpPWM_Pin_2 19
#define toFPin 0
//define shut_toF 23
#define DHTpin 2


// I2C Busses
#define SDA1 21
#define SCL1 22
#define SDA2 5
#define SCL2 18

// Multiplexer
#define S0 27
#define S1 26
#define S2 25
#define S3 33
#define SIG 32

// L298N
//#define enA 34
//#define in1 35
//#define in2 32

#elif defined(ESP8266)
#define DEVICE "ESP8266"
#define wakeUpPin 16 //D0
#define pumpButtonPin 5 //D1
#define relaisPin 4 //D2 
#define toFPin 0 //D3 
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
#define C15 15

#endif //pins_h