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

unsigned long previousMillis = 0;
const int SLEEP_INTERVAL = 5000;
const int STATE_DELAY = 1000;

void on_initState(){
  if(fsm.executeOnce){
    Serial.println("init State once");
    if(!services.GetWifiStatus())
    {
      services.SetupWifi();
    }
    if(!influxHelper.CheckInfluxConnection())
    {
        influxHelper.SetupInflux();
    }
  }
  //Serial.println("init State");
}

void on_sleepState(){
  if(fsm.executeOnce){
    Serial.println("sleep State once");
    previousMillis = millis();
  }
  //Serial.println("sleep State");
}

void on_measureState(){
  if(fsm.executeOnce){
    Serial.println("measure State once");
    delay(2000);
    //WiFi.disconnect();
  }
  //Serial.println("measure State");
}

bool transitionS0S0(){
  if(digitalRead(5) == HIGH){
    return true;
  }
  return false;
}

bool transitionS0S1(){
  if(influxHelper.CheckInfluxConnection()){
    return true;
  }
  return false;
}

bool transitionS1S2(){
  Serial.println(millis());
  if(millis() - previousMillis >= SLEEP_INTERVAL)
  {
    return true;
  }
  return false;
}

// Regularly check in measureState if any Connection
// is lost, if true go back to initState
bool transitionS2S0(){
  if(!influxHelper.CheckInfluxConnection() || !services.GetWifiStatus()){
    return true;
  }
  return false;
}

/*
bool transitionS2S2(){
  // Falls Länge der Map/Liste vollständig prüfe alle Einträge,
  // falls einer nan durchlaufe S2 erneut
}
*/


void setup() {

  Serial.begin(9600);

  climate1.InitialiseDHT();


  State* initState = fsm.addState(&on_initState);
  State* sleepState = fsm.addState(&on_sleepState);
  State* measureState = fsm.addState(&on_measureState);
  //initState->addTransition(&transitionS0S0,initState);
  initState->addTransition(&transitionS0S1,sleepState);
  sleepState->addTransition(&transitionS1S2, measureState);
  measureState->addTransition(&transitionS2S0,initState);
}

void doMeasurements()
{
  //DHTdata m = climate1.MeasureDHT();
  float lightVal = fotoresistor1.measureLight();


  influxHelper.WriteDataPoint(lightVal);
}


void loop() {

  fsm.run();
  delay(STATE_DELAY);
}