#ifndef main_h
#define main_h

#include <Arduino.h>
#include <Wire.h>
#include <influxHelper.h>
#include <pins.h>

extern TwoWire I2Cone;
extern TwoWire I2Ctwo;
extern InfluxHelper influxHelper;
extern bool wateringNeeded;

#endif //main_h