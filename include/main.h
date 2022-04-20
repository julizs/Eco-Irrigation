#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include <Multiplexer.h>
#include <InfluxHelper.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>

#define SLEEPTYPE 0 // No/Light/Modem/... Sleep
#define DO_MEASURE 1
#define TRANSMIT_DATA 1
#define REQUEST_DATA 1
#define RUN_SUBMACHINES 1
extern uint8_t SLEEP_DUR, IDLE_DUR, MIN_STATE_DUR;
extern TwoWire I2Cone, I2Ctwo;
extern InfluxHelper influxHelper;
extern LinkedList<ISubStateMachine*> actions;
// extern DynamicJsonDocument moistureSensors, plants, plantNeeds, pumps;

extern const char baseUrl[];

// watchDog checks Critical Errors, goto ERROR State
extern uint8_t critErrCode;
extern const char* critErrMessage[];

#endif //main_h