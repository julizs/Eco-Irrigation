#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include <Multiplexer.h>
#include <InfluxHelper.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>

// Disable Brownout Warnings
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define SLEEPTYPE 0 // No/Light/Modem/... Sleep
#define DO_MEASURE 1
#define TRANSMIT_DATA 1
#define REQUEST_DATA 1
#define RUN_SUBMACHINES 1
extern uint8_t SLEEP_DUR, IDLE_DUR, STATE_MIN_DUR;
extern LinkedList<String> transDestinations;

extern TwoWire I2Cone, I2Ctwo;
extern LinkedList<ISubStateMachine*> actions;

extern const char baseUrl[];

extern char *critErrMessage;
extern uint8_t critErrCode;

#endif //main_h