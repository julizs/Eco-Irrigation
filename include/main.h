#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include <Multiplexer.h>
#include <InfluxHelper.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>
#include <PowerMeter.h>
// Disable Brownout Warnings
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Forward Declarations
class PowerMeter; 
class FlowMeter;

// Commonly used Sensor Objects
extern PowerMeter powerMeter1;
// extern FlowMeter flowMeter1;

// #define SLEEPTYPE 0 // No/Light/Modem/... Sleep
#define DO_MEASURE 1
#define TRANSMIT_DATA 1
#define REQUEST_DATA 1
#define RUN_SUBMACHINES 1

extern TwoWire I2Cone, I2Ctwo;
extern uint8_t SLEEP_TYPE, SLEEP_DUR, IDLE_DUR, STATE_MIN_DUR;
extern bool hwButtonsEnabled;
extern LinkedList<String> manualTransitions;
// extern LinkedList<ISubStateMachine*> actions;

extern const char baseUrl[];

extern const char *const critErrMessages[];
extern uint8_t critErrCode;

#endif //main_h