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
#include <multiplexer.h>

Services services;
InfluxHelper influxHelper;

Multiplexer multi1;
Climate climate1(500, 2);
SoilMoisture soilMoisture1(550, 10, C0);
Fotoresistor fotoResistor1(10000, 3.3, 10, C1);
Pump::PumpModel qr50e(12, 12, 5, 240);
Pump::PumpModel palermo(6, 12, 5, 330);
Pump pump1(qr50e);

Plant plant2(fotoResistor1, soilMoisture1);
std::vector<Plant> plants{plant2};

StateMachine fsm = StateMachine();
State *initState, *sleepState, *measureState, *evaluateState, *actionState;

// Debouncing
unsigned long lastDebounceTime = 0; // Last time Output Pin was toggled
unsigned long debounceDelay = 50;   // Increase if Output flickers
int buttonState;
int lastButtonState = HIGH; // Initial State is Off
//

bool wateringNeeded, didSleep, didMeasure, didCycle;
unsigned long stateBeginMillis = 0;
const int SLEEP_INTERVAL = 5;
const int MEASURE_INTERVAL = 2;
const int LOOP_DELAY = 500; // Maschine evaluates Logic in States only every 500ms

void doMeasurements()
{
  didMeasure = false;
  // Measurements per EcoBox
  byte rssi = WiFi.RSSI();
  climate1.loop();
  //DHTdata data = climate1.loop();

  // Plant-specific Measurements
  for (auto &plant : plants)
  {
    plant.measureSensors();
    //Serial.println(plant.lightSensor.measureLight());
  }

  // Unessesary wait...
  // Better: One bool, all Funcs should set it true when ready,
  // and Classes/Plants submit data directly to InfluxDb
  if (millis() - stateBeginMillis >= MEASURE_INTERVAL)
  {
    // Goto next State
    didMeasure = true;
  }

  /* if(climate1.measurementsComplete && )
  {
    measurementsComplete = true;
  } */
}

void doEvaluate()
{
  wateringNeeded = true;
  didCycle = true;
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
  if (reading != lastButtonState)
  {

    // Reset Debouncing Timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {

    if (reading != buttonState)
    {
      buttonState = reading;

      if (buttonState == LOW)
      {
        Serial.println("Pump Button pressed!");
        wateringNeeded = true;
        fsm.transitionTo(actionState);
      }
    }
  }

  lastButtonState = reading;
}

void checkConnections()
{
  if (!influxHelper.checkInfluxConnection() || !services.getWifiStatus())
  {
    fsm.transitionTo(initState);
  }
}

// STATE LOGIC
void on_initState()
{
  if (fsm.executeOnce)
  {
    Serial.println("init State once");
    stateBeginMillis = millis();

    if (!services.getWifiStatus())
    {
      services.setupWifi();
    }
    if (!influxHelper.checkInfluxConnection())
    {
      influxHelper.setupInflux();
    }
  }
}

void on_sleepState()
{
  if (fsm.executeOnce)
  {
    didSleep = false;
    Serial.println("sleepState once");
    stateBeginMillis = millis();
    ESP.deepSleep(30e6); // Connect Sleep Cable AFTER Uploading Code
    checkConnections();
    didSleep = true;
  }
  /*
  Serial.print(".");
  if(millis() - stateBeginMillis >= SLEEP_INTERVAL * 1000UL)
  {
    didSleep = true;
  }
  */
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    Serial.println("measureState once");
    stateBeginMillis = millis();
    //WiFi.disconnect();
    checkConnections();
    doMeasurements();
  }
}

void on_evaluateState()
{
  if (fsm.executeOnce)
  {
    Serial.println("evaluateState once");
    stateBeginMillis = millis();
    checkConnections();
    doEvaluate();
  }
}

// Pump, Fan, Heater, ...
void on_actionState()
{
  if (fsm.executeOnce)
  {
    Serial.println("watering State once");
    stateBeginMillis = millis();
    pump1.doPump = true;
  }
  // run sub-StateMachines in loop
  if(wateringNeeded)
  {
    pump1.loop();
  }
  //if(coolingNeeded) {...}
}

// TRANSITION LOGIC
// Funcs are evaluated constantly in current State
bool transitionS0S1()
{
  if (didCycle)
  {
    return true;
  }
  return false;
}

bool transitionS0S2()
{
  if (!didCycle)
  {
    return true;
  }
  return false;
}

bool transitionS1S2()
{
  if (didSleep)
  {
    return true;
  }
  return false;
}

bool transitionS2S3()
{
  if (didMeasure)
  {
    return true;
  }
  return false;
}

bool transitionS3S1()
{
  if (!wateringNeeded)
  {
    return true;
  }
  return false;
}

bool transitionS3S4()
{
  if (wateringNeeded)
  {
    return true;
  }
  return false;
}

bool transitionS4S1()
{
  if(!pump1.doPump)
  {
    return true;
  }
  return false;
}

void setup()
{

  // Wait for serial to init
  while (!Serial){}
  Serial.begin(9600);

  climate1.setup();

  pinMode(pumpButtonPin, INPUT);
  pinMode(relaisPin, OUTPUT);
  pinMode(laserDistPin, INPUT);

  multi1.setup();

  initState = fsm.addState(&on_initState);
  sleepState = fsm.addState(&on_sleepState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  actionState = fsm.addState(&on_actionState);

  initState->addTransition(&transitionS0S1, sleepState);
  initState->addTransition(&transitionS0S2, measureState);
  sleepState->addTransition(&transitionS1S2, measureState);
  measureState->addTransition(&transitionS2S3, evaluateState);
  evaluateState->addTransition(&transitionS3S4, actionState);
  actionState->addTransition(&transitionS4S1, sleepState);
}

void loop()
{

  fsm.run();
  //delay(LOOP_DELAY);

  // Check constantly in all States and Sub StateMachines:
  checkButtons();
}

/* const Pump::PumpModel pumpModel = pump1.getPumpModel();
    Serial.println(pumpModel.minVoltage);
    pump1.setPumpModel(palermo);
    const Pump::PumpModel pumpModelNew = pump1.getPumpModel();
    Serial.println(pumpModelNew.minVoltage); 
*/

/*
  Execution interval of logic in any transition func 
  or logic in any state is dependant on delay(LOOP_DELAY)
  Put in any TransitionLogic Func:
  if(millis() - stateBeginMillis >= SLEEP_INTERVAL)
*/