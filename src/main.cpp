#include <Arduino.h>
#include <Wire.h>
#include <climate.h>
#include <soilmoisture.h>
#include <fotoresistor.h>
#include <pins.h>
#include <services.h>
#include <influxHelper.h>
#include <LinkedList.h>
#include <StateMachine.h>


Climate climate1(4);
SoilMoisture soilMoisture1(550);
Fotoresistor fotoresistor1(10000, 3.3, analogPin);
Services services;
InfluxHelper influxHelper;
StateMachine fsm = StateMachine();

unsigned long startMillis = 0;
const int INTERVAL = 1000;
const int STATE_DELAY = 1000;

void on_initState(){
  if(fsm.executeOnce){
    Serial.println("init State once");
    //digitalWrite(LED,!digitalRead(LED));
  }
  Serial.println("init State");
}

void on_sleepState(){
  if(fsm.executeOnce){
    Serial.println("sleep State once");
    //digitalWrite(LED,!digitalRead(LED));
  }
  Serial.println("sleep State");
}

bool transitionS0S0(){
  if(digitalRead(5) == HIGH){
    return true;
  }
  return false;
}

bool transitionS0S1(){
  Serial.println("Trans S0S1 auswerten");
  if(influxHelper.CheckInfluxConnection() == 1){
    return true;
  }
  return false;
}


void setup() {

  Serial.begin(9600);

  // SetupSensors();
  climate1.InitialiseDHT();

  services.SetupWifi();
  influxHelper.SetupInflux();
  //influxHelper.CheckInfluxConnection();

  State* initState = fsm.addState(&on_initState);
  State* sleepState = fsm.addState(&on_sleepState);
  //initState->addTransition(&transitionS0S0,initState);
  initState->addTransition(&transitionS0S1,sleepState);
}

void doMeasurements()
{
  DHTdata m = climate1.MeasureDHT();
  float lightVal = fotoresistor1.measureLight();


  influxHelper.WriteDataPoint(lightVal);
}


void loop() {

  fsm.run();
  delay(STATE_DELAY);
}