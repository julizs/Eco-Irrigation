#ifndef Pins_h
#define Pins_h

#if defined(ESP32)
//#include <WiFiMulti.h>
//WiFiMulti wifiMulti;
#define DEVICE "ESP32"

#define button1Pin 34
//#define button2Pin 4
//#define button3Pin 19
#define pumpPWM_Pin_1 4 // 23
#define pumpPWM_Pin_2 2 // 19
#define shut_toF 19 // 4

// I2C Busses
#define SDA1 21
#define SCL1 22
#define SDA2 5
#define SCL2 18

// Dotmatrix (Max7219)
#define DATA 13
#define CS 12
#define CLK 14

// Multiplexer
#define S0 27
#define S1 26
#define S2 25
#define S3 33
#define SIG 32

// DHT22 und MH-Z19 PWM
// (Pins 34 and 35 can only be input, no output)
// #define DHT_IN 34
#define MHZ19_PWM 35

// Relais
#define RELAIS_1 15
#define RELAIS_2 23

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

#endif // Pins_h