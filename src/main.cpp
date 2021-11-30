#include <Arduino.h>
#include <Wire.h>
#include <climate.h>
#include <soilmoisture.h>
#include <fotoresistor.h>
#include <pins.h>
#include <services.h>
#include <influxHelper.h>


Climate climate1(4);
SoilMoisture soilMoisture1(550);
Fotoresistor fotoresistor1(10000, 3.3, analogPin);
Services services;
InfluxHelper influxHelper;


void setup() {

  delay(200);

  Serial.begin(9600);

  services.SetupWifi();

  // SetupSensors();
  climate1.InitialiseDHT();

  influxHelper.SetupInflux();

  influxHelper.CheckInfluxConnection();
}

void DoMeasure()
{
  DHTdata m = climate1.MeasureDHT();
  float lightVal = fotoresistor1.measureLight();
  influxHelper.WriteDataPoint(lightVal);
}


void loop() {

  delay(2000);
  DoMeasure();
  //influxHelper.WriteDataPoint();
}



