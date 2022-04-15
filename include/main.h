#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include <Multiplexer.h>
#include <InfluxHelper.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>

#define SLEEPTYPE 0 // Light Sleep or Modem Sleep
#define LOCALSAVE 1 // Save ArduinoJson Files on Flash
#define DOMEASURE 1
#define SENDDATA 1
#define GETDATA 1
#define RUNSUBMACHINES 0
extern uint8_t SLEEP_DUR, IDLE_DUR; // Changable by User
extern uint8_t MIN_STATE_DUR;
extern TwoWire I2Cone;
extern TwoWire I2Ctwo;
extern InfluxHelper influxHelper;
extern LinkedList<ISubStateMachine*> actions;
// extern DynamicJsonDocument moistureSensors, plants, plantNeeds, pumps;

extern const char baseUrl[];

// watchDog checks Critical Errors, goto ERROR State
extern uint8_t critErrCode;
extern const char* critErrMessage[];

#endif //main_h