#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include <Multiplexer.h>
#include <Services.h>
#include <InfluxHelper.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>

#define SLEEPTYPE 1 // Light Sleep or Modem Sleep
#define LOCALSAVE 1 // Save ArduinoJson Files on Flash
#define DOMEASURE 1
#define SENDDATA 1
#define GETDATA 1
#define RUNSUBMACHINES 1
const int SLEEP_DURATION = 16;
const int MEASURE_INTERVAL = 2;
const int MIN_STATE_DURATION = 2;
extern TwoWire I2Cone;
extern TwoWire I2Ctwo;
extern InfluxHelper influxHelper;
extern const char baseUrl[];
extern LinkedList<ISubStateMachine*> actions;

#endif //main_h