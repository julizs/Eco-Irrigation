#include <Arduino.h>
#include <Wire.h>
#include <plant.h>
#include <climate.h>
#include <soilmoisture.h>
#include <fotoresistor.h>
#include <pins.h>
#include <services.h>
#include <influxHelper.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <pump.h>

Services services;
InfluxHelper influxHelper;

Climate climate1(500,2);
SoilMoisture soilMoisture1(550, 10, C0);
Fotoresistor fotoResistor1(10000, 3.3, 10, C1);
Pump::PumpModel qr50e(12, 12, 5, 240);
Pump::PumpModel palermo(6, 12, 5, 330);
Pump pump1(qr50e);

Plant plant2(fotoResistor1, soilMoisture1);
std::vector<Plant> plants {plant2};

StateMachine fsm = StateMachine();
State *initState, *sleepState, *measureState, *evaluateState, *wateringState;

// Debouncing
unsigned long lastDebounceTime = 0;  // Last time Output Pin was toggled
unsigned long debounceDelay = 50;    // Increase if Output flickers
int buttonState;
int lastButtonState = HIGH; // Initial State is Off
//

bool measurementsComplete, wateringNeeded;
unsigned long stateBeginMillis = 0;
const int SLEEP_INTERVAL = 1000;
const int LOOP_DELAY = 500; // Maschine evaluates Logic in States only every 500ms




void doMeasurements()
{
  // Global Measurements
  byte rssi = WiFi.RSSI();
  DHTdata data = climate1.loop();
  
  // Plant-specific Measurements
  for(auto& plant: plants) {
    plant.measureSensors();
    //Serial.println(plant.lightSensor.measureLight());
  }
  measurementsComplete = true;
}

void doEvaluate()
{
  wateringNeeded = true;
  // influxHelper.WriteDataPoint(lightVal);
}

void checkButtons()
{
  // Debouncing
  int reading = digitalRead(pumpButtonPin);

  /*
  if(reading == LOW)
  {
    Serial.println("Pump Button pressed!");
  }
  */
  
  // If Switch changed, due to Noise or Pressing:
  if (reading != lastButtonState) {
    
    // Reset Debouncing Timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        Serial.println("Pump Button pressed!");
        fsm.transitionTo(wateringState);
      }
    }
  }
  
  lastButtonState = reading;
}



// STATE LOGIC
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
  pump1.loop(); // run StateMachine of class Pump
}




// TRANSITION LOGIC 
// Funcs get evaluated constantly in current State
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

// Any Connection lost? -> go back to initState
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

void setup() {

  Serial.begin(9600);

  climate1.setup();

  pinMode(pumpButtonPin, INPUT);

  initState = fsm.addState(&on_initState);
  sleepState = fsm.addState(&on_sleepState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  wateringState = fsm.addState(&on_wateringState);

  initState->addTransition(&transitionS0S1,sleepState);
  sleepState->addTransition(&transitionS1S2, measureState);
  measureState->addTransition(&transitionS2S0,initState);
  measureState->addTransition(&transitionS2S3,evaluateState);
  evaluateState->addTransition(&transitionS3S4,wateringState);
}

void loop() {

  fsm.run();
  //delay(LOOP_DELAY);
  checkButtons();
}