#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>

#include <Multiplexer.h>
#include <Services.h>
#include <InfluxHelper.h>

extern TwoWire I2Cone;
extern TwoWire I2Ctwo;
extern InfluxHelper influxHelper;
extern Services services;
extern bool wateringNeeded;

#endif //main_h