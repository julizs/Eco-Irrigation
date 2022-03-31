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

extern TwoWire I2Cone;
extern TwoWire I2Ctwo;
extern InfluxHelper influxHelper;
extern bool wateringNeeded;
extern const char baseUrl[];
extern LinkedList<ISubStateMachine*> actions;

#endif //main_h