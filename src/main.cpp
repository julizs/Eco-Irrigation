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
#include <pump.h>


Climate climate1(500,2);
SoilMoisture soilMoisture1(550);
Fotoresistor fotoresistor1(10000, 3.3, analogPin);
Services services;
InfluxHelper influxHelper;
Pump::PumpModel qr50e(12, 12, 15, 240);
Pump::PumpModel palermo(6, 12, 15, 330);
Pump pump1(qr50e);
StateMachine fsm = StateMachine();

bool measurementsComplete, wateringNeeded;
unsigned long stateBeginMillis = 0;
const int SLEEP_INTERVAL = 10000;
const int STATE_DELAY = 500;

void doMeasurements()
{
  measurementsComplete = true;
  // foreach plant in plants...
  // multiple measure attempts only for some values (dht11), NOT for smoothed ones like lightVal
  
  /* DHTdata m = climate1.MeasureDHT();
  float lightVal = fotoresistor1.measureLight();
  float soilMoisture = soilMoisture1.SmoothedSoilMoisture();
  byte rssi = WiFi.RSSI(); */
}

void doEvaluate()
{
  wateringNeeded = true;
  // influxHelper.WriteDataPoint(lightVal);
}

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
    Serial.println("Sleeping");
    stateBeginMillis = millis();
    /* const Pump::PumpModel pumpModel = pump1.getPumpModel();
    Serial.println(pumpModel.minVoltage);
    pump1.setPumpModel(palermo);
    const Pump::PumpModel pumpModelNew = pump1.getPumpModel();
    Serial.println(pumpModelNew.minVoltage); */
    //ESP.deepSleep(30e6);
  }
  Serial.print(".");
}

void on_measureState(){
  if(fsm.executeOnce){
    Serial.println();
    Serial.println("measure State once");
    //WiFi.disconnect();
    doMeasurements();
  }
}

void on_evaluateState(){
  if(fsm.executeOnce){
    Serial.println("evaluate State once");
    doEvaluate(); // Execute Once
  }
}

void on_wateringState(){
  if(fsm.executeOnce){
    Serial.println("watering State once");
    pump1.setup();
    pump1.doPump = true;
  }
  pump1.loop(); // run/delegate to StateMachine of class Pump
}

// Transition Funcs get evaluated constantly (or each STATE_DELAY tick)
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
  //Serial.println(millis());
  if(millis() - stateBeginMillis >= SLEEP_INTERVAL)
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

bool transitionS2S3(){
  if(measurementsComplete){
    return true;
  }
  return false;
}

bool transitionS3S4(){
  if(wateringNeeded){
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
  State* evaluateState = fsm.addState(&on_evaluateState);
  State* wateringState = fsm.addState(&on_wateringState);
  //initState->addTransition(&transitionS0S0,initState);
  initState->addTransition(&transitionS0S1,sleepState);
  sleepState->addTransition(&transitionS1S2, measureState);
  measureState->addTransition(&transitionS2S0,initState);
  measureState->addTransition(&transitionS2S3,evaluateState);
  evaluateState->addTransition(&transitionS3S4,wateringState);
}

void loop() {

  fsm.run();
  delay(STATE_DELAY);
}


/*
TODO
Pumpe darf nur ein PUMP_INVERVALL lang an sein, dann Wechsel in anderen State
Schnelles Messen auf Befehl
*/