#include <main.h>
#include <plant.h>
#include <climate.h>
#include <soilmoisture.h>
#include <pump.h>
#include <multiplexer.h>
#include <services.h>
#include <influxHelper.h>
#include <LinkedList.h>
#include <StateMachine.h>


Services services;
InfluxHelper influxHelper;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

Climate climate1(500, 2);
SoilMoisture soilMoisture1(550, 10, C0);
//Fotoresistor fotoResistor1(10000, 3.3, 10, C15);
AmbientLight lightSensor1(2591,I2Ctwo); // TSL2591
Pump::PumpModel qr50e(12, 12, 2, 240);
Pump::PumpModel palermo(6, 12, 2, 330);
Pump pump1(qr50e);
Plant plant2(lightSensor1, soilMoisture1);
std::vector<Plant> plants{plant2};

StateMachine fsm = StateMachine();
State *initState, *sleepState, *measureState, *evaluateState, *actionState;
const char *stateNames[] = {"INIT", "SLEEP", "MEASURE", "EVALUATE", "ACTION"};
bool wateringNeeded, didSleep, didCycle;
unsigned long stateBeginMillis = 0;
const int SLEEP_INTERVAL = 4;
const int MEASURE_INTERVAL = 2;
const int MIN_STATE_DURATION = 2;

// Debouncing
unsigned long lastDebounceTime = 0; // Last time Output Pin was toggled
unsigned long debounceDelay = 50;   // Increase if Output flickers
int buttonState;
int lastButtonState = HIGH; // Initial State is Off


void scanI2CBus(TwoWire *wire)
{
  Serial.println("Scanning I2C Addresses Channel 1");
  uint8_t cnt=0;
  for(uint8_t i=0;i<128;i++){
    wire->beginTransmission(i);
    uint8_t ec=wire->endTransmission(true);
    if(ec==0){
      if(i<16)Serial.print('0');
      Serial.print(i,HEX);
      cnt++;
    }
    else Serial.print("..");
    Serial.print(' ');
    if ((i&0x0f)==0x0f)Serial.println();
    }
  Serial.print("Scan Completed, ");
  Serial.print(cnt);
  Serial.println(" I2C Devices found.");
}

void doMeasurements()
{
  // Global Measurements per EcoBox

  // Esp32 Main:
  scanI2CBus(&I2Cone);
  scanI2CBus(&I2Ctwo);
  
  pump1.setupToF();
  int waterLevel = pump1.readToF();
  Serial.println("Wasserstand: " + waterLevel);

  byte rssi = WiFi.RSSI();
  Serial.println("Rssi: " + rssi);

  lightSensor1.setupTSL2591();
  TSL2591data data = lightSensor1.measureLight();
  
  influxHelper.writeDataPoint(p0, "rssi", rssi);
  influxHelper.writeDataPoint(p2, "waterLevel", waterLevel);
  influxHelper.writeDataPoint(p3, "Infrared Light", data.infraRed);
  influxHelper.writeDataPoint(p3, "Visible Light", data.visibleLight);

  //Esp32 Cam:
  //climate1.loop();
  //DHTdata data = climate1.loop();




  // Plant-specific Measurements
  for (auto &plant : plants)
  {
    //plant.measureSensors();
    //Serial.println(plant.lightSensor.measureLight());
  }
}

void doEvaluate()
{
  wateringNeeded = true;
  didCycle = true;
  // influxHelper.WriteDataPoint(lightVal);
}

void checkButtons()
{
  int reading = digitalRead(button1Pin);

  // If Switch changed, due to Noise or Pressing:
  if (reading != lastButtonState)
  {
    // Reset Debouncing Timer
    lastDebounceTime = millis();
  }

  // Debouncing
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;

      if (buttonState == LOW)
      {
        Serial.println("Pump Button pressed!");
        
        wateringNeeded = true; // Go to actionState
        pump1.pumpSignal = true; // Turn Pump on/off cause of Button

        if(fsm.currentState != 4) // otherwise ACTION state before PUMP_IDLE when pressed while Pumping
        {
          fsm.transitionTo(actionState);
        }  
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

void commonStateLogic()
{
  Serial.println(stateNames[fsm.currentState]);
  //checkConnections();
  stateBeginMillis = millis();
}

bool countTime(int duration)
{
  return (millis() - stateBeginMillis >= duration * 1000UL);
}

// STATE LOGIC
void on_initState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

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
    commonStateLogic();
    //ESP.deepSleep(30e6); // Connect Sleep Cable AFTER Uploading Code
    //checkConnections();
    //didSleep = true;
  }
  // Simulate Sleep
  // Serial.print(".");
  if(millis() - stateBeginMillis >= SLEEP_INTERVAL * 1000UL)
  {
    didSleep = true;
  }
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    //WiFi.disconnect();
    doMeasurements();
  }
}

void on_evaluateState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    doEvaluate();
  }
}

// Pump, Fan, Heater, ...
void on_actionState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    // Turn Pump on cause of Measurements
    pump1.pumpSignal = true;
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
  if (countTime(MIN_STATE_DURATION) && didCycle) // Sleep if measured before
  {
    return true;
  }
  return false;
}

bool transitionS0S2()
{
  if (countTime(MIN_STATE_DURATION) && !didCycle) // Measure immediately if it didnt yet
  {
    return true;
  }
  return false;
}

bool transitionS1S2()
{
  if (countTime(MIN_STATE_DURATION) && didSleep)
  {
    return true;
  }
  return false;
}

bool transitionS2S3()
{
  //*
  // Wait till max Measure time is up || dht11.didMeasure && aht10.didMeasure && ... (Sensors ready before)
  if (countTime(MEASURE_INTERVAL) || climate1.measurementsComplete)
  {
    return true;
  }
  return false;
}

bool transitionS3S1()
{
  if (countTime(MIN_STATE_DURATION && !wateringNeeded)) // && !coolingNeeded && ...
  {
    return true;
  }
  return false;
}

bool transitionS3S4()
{
  if (countTime(MIN_STATE_DURATION && wateringNeeded)) // || coolingNeeded || ...
  {
    return true;
  }
  return false;
}

bool transitionS4S1()
{
  // pump1.pumpDone && fan1.fanDone && ... || millis() - ... (max. Einschaltzeit)
  // if(countTime(MIN_STATE_DURATION) && pump1.currentState == PumpState::IDLE && !pump1.pumpSignal)
  if(countTime(MIN_STATE_DURATION) && pump1.cycleDone)
  {
    return true;
  }
  return false;
}

void setup()
{

  // Wait for serial to init
  // while (!Serial){}
  Serial.begin(115200);

  //Wire.begin();
  I2Cone.begin(SDA1,SCL1,200000);
  I2Ctwo.begin(SDA2,SCL2,100000);
  delay(500);
  
  /*
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);

  climate1.setup();
  Multiplexer::setup();
  */
  

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

/* 
  const Pump::PumpModel pumpModel = pump1.getPumpModel();
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

/*
  //*
  Vorteile: 
  Alle bool Flags sind je in Klasse, verstopfen main.cpp nicht
  maxPollTime und maxPumpTime je einzeln in Klassen festlegbar/prüfbar
  statt einzelne festes Zeitlimit für alle
  Nachteile:
  Prüfung aller bools in jedem cycle, extrem oft, besser wenn Prüfung
  aller bools nur je, wenn eine der Funktionen zurückkehrt / Event
  Dazu müssen Funktionen von main.cpp von anderen Klassen aufrufbar sein
  */