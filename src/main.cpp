#include <main.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <ArduinoJson.h>
#include <Irrigation.h>
#include <Utilities.h>
#include <AmbientClimate.h>
#include <SoilMoisture.h>
#include <AmbientLight.h>
#include <StatusDisplay.h>
#include <ButtonHandler.h>
#include <Pump.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>

const char baseUrl[] = "https://juli.uber.space/node";
uint8_t IDLE_DUR = 4;
uint8_t SLEEP_DUR = 16;
uint8_t STATE_MIN_DUR = 6;

TaskHandle_t Task1, Task2;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

AmbientClimate climate1(500, 2);
AmbientLight lightSensor1(1);
AmbientLight lightSensor2(2);
StatusDisplay displayController;

// Pumps and associated solenoids/relaisChannels and ToF Sensor never change,
// only associated Pump Model (if User switches Pumps)
uint8_t solenoids1[] = {0, 1};
uint8_t solenoids2[] = {};
Cistern cistern1(0x51, solenoids1, 300, 53.0f);
Cistern cistern2(0x52, solenoids2, 450, 42.0f);
Pump pump1(0, pump_PWM_1, cistern1);
Pump pump2(1, pump_PWM_2, cistern2);

StateMachine fsm = StateMachine();
State *nextState = nullptr;
uint8_t critErrCode = 0;
const char *critErrMessage[] = {"None", "Final Fail to connect WiFi", "Final Fail to setup ToFs", "Final Fail to send Buffer"};
State *idleState, *initState, *connectState, *requestState, *measureState, *evaluateState, *actionState, *transmitState, *errorState;
const char *stateNames[] = {"IDLE", "INIT", "CONNECT", "REQUEST", "MEASURE", "EVALUATE", "ACTION", "TRANSMIT", "ERROR"};
bool didSleep, sensorsReady, didConnect, didRequest,  didActions, didTransmit;
uint8_t selfTrans = 0, maxSelfTrans = 3, waitTime = 2;
uint32_t stateBeginMillis = 0;
// DynamicJsonDocument moistureSensors(2048), plants(2048), plantNeeds(2048), pumps(2048);

bool countTime(int durationSec)
{
  return (millis() - stateBeginMillis >= durationSec * 1000UL);
}

// Always setup both Sensors at once
void setupToFs()
{
  while (!cistern2.setupToF())
    ;
  while (!cistern1.setupToF())
    ;
}

void doMeasurements()
{
  // ESP32 GLOBAL MEASUREMENTS
  if (!p0.hasTags()) // Reuse Datapoint
  {
    p0.addTag("device", DEVICE);
    p0.addTag("SSID", WiFi.SSID());
  }
  p0.clearFields();

  if (cistern1.toF_ready)
    cistern1.updateWaterLevel();

  if (cistern2.toF_ready)
    cistern2.updateWaterLevel();

  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  InfluxHelper::writeDataPoint(p0);

  // PLANT-SPECIFIC MEASUREMENTS
  // Advantage: Local DynamicJsonDocs (big) get destroyed after leaving Scope
  DynamicJsonDocument plants(2048), moistureSensors(1024);
  Services::doJSONGetRequest("/plants/sensors", plants);
  Services::doJSONGetRequest("/moistureSensors", moistureSensors);

  Point p("Plant Data");
  int lastMeasuredLightSensor = -1;
  TSL2591data data;

  // For each Plant, measure (only) assigned Sensors
  for (int i = 0; i < plants.size(); i++)
  {
    String plantName = plants[i]["name"].as<String>();
    Serial.print("Plant: ");
    Serial.println(plantName);

    // Reuse Datapoint, write new data point/row into this table
    p.clearTags();
    p.clearFields();
    p.addTag("Plant", plantName);

    // New way since ArduinoJson 6
    // StaticJsonDocument<64> doc;
    JsonArray array = plants[i]["moistureSensors"];
    if (array.isNull())
    {
      Serial.println("Plant has no moisture Sensors.");
      continue;
    }

    // For each moistureSensor assigned to this Plant
    for (int j = 0; j < array.size(); j++)
    {
      int moistureSensor = array[j];
      int pinNum = moistureSensor - 1;
      char key[64];
      snprintf(key, 64, "Soil Moisture Sensor %d", moistureSensor);

      int moistureSmoothed = SoilMoisture::measureSoilMoistureSmoothed(pinNum);
      // Pass Reference of moistureSensors Table, so only 1 GET Request instead of per Plant, per Sensor
      int moisturePercentage = SoilMoisture::voltageToPercentage(pinNum, moistureSmoothed, moistureSensors);
      p.addField(key, moisturePercentage);
    }

    // MongoDb Cursor gets sorted by lightSensor, same Sensor measures less often
    int lightSensor = plants[i]["lightSensor"].as<int>();

    if (lightSensor != lastMeasuredLightSensor)
    {
      data = lightSensor2.measureLight();
      lastMeasuredLightSensor = lightSensor;
    }
    p.addField("Infrared Light", data.infraRed);
    p.addField("Visible Light", data.visibleLight);

    InfluxHelper::writeDataPoint(p);
  }
  // Serial.printf_P(PSTR("free heap memory: %d\n"), ESP.getFreeHeap());
}

void commonStateLogic()
{
  stateBeginMillis = millis();

  Serial.println();
  Serial.println(stateNames[fsm.currentState]);

  // Safety
  pump1.switchOff();
  pump2.switchOff();
  digitalWrite(Relais[0], HIGH);
  digitalWrite(Relais[1], HIGH);
}

/*
https://lastminuteengineers.com/esp32-sleep-modes-power-consumption/
https://m1cr0lab-esp32.github.io/sleep-modes/
1 = Low to High, 0 = High to Low. Pin pulled HIGH
esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);
Sleep (whatever Type) happened in executeOnce, no need to wait for MIN_STATE_DUR
*/
void on_idleState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    // ESP.deepSleep(30e6);

    if (SLEEPTYPE == 1) // Light Sleep
    {
      Serial.println("Entering Light Sleep.");
      esp_sleep_enable_timer_wakeup(SLEEP_DUR * 1000 * 1000); // µs
      delay(100);                                             // Else no Wakeup
      esp_light_sleep_start();
      // Resume Program, Connections and States were kept
      nextState = initState;
    }
    else if (SLEEPTYPE == 2)
    {
      // Deep Sleep
    }
  }

  if (SLEEPTYPE == 0) // Simulate Sleep / Idle
  {
    // Serial.println("Idle");
    while (!countTime(IDLE_DUR))
    {
    }
  }
  nextState = initState;
}

/*
Run setups, wait for MIN_STATE_DUR (non-blocking), check if ready
TODO: improve ToF Setup
*/
void on_initState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    sensorsReady = false;
    
    Serial.print("Main State Machine runs on Core: ");
    Serial.println(xPortGetCoreID());

    // Setup Sensors
    setupToFs();
    pump1.setupIna();
    lightSensor1.setupTSL2591(I2Cone);
    lightSensor2.setupTSL2591(I2Ctwo);
    // sensorsReady = lightSensor1.isReady();

    Utilities::scanI2CBus(&I2Cone);
    Utilities::scanI2CBus(&I2Ctwo);

    setCpuFrequencyMhz(80); // Clock down, prevent Brownout
  }

  if(countTime(STATE_MIN_DUR))
  {
    sensorsReady = pump1.inaReady();
    sensorsReady = lightSensor2.isReady();

    if(sensorsReady)
    {
        selfTrans = 0;
        nextState = connectState;  
    }
    else if(selfTrans < maxSelfTrans)
    {
        selfTrans++;
        nextState = initState;
    }
    else
    {
        critErrCode = 3;
    }
  }
}

/* "Active" Button Handling
Send current own IPv4/Ipv6
Services::doPostRequest("/commands/ip");
// WiFi.begin() before this, or Exception
// Restart / ask for Status?
Services::startRestServer();
Influx Params must be set before sending Data to InfluxDB, or
Buffer Settings etc. won't be applied
*/
void on_connectState()
{
  if (fsm.executeOnce)
  {
    didConnect = false;
    commonStateLogic();

    if (!Services::wifiConnected())
      Services::setupWifi();

    // Establish and hold InfluxDB Connection open
    didConnect = InfluxHelper::checkConnection();
    if(!didConnect)
      while (!InfluxHelper::setParameters()); 
  }
 
  if(countTime(STATE_MIN_DUR))
  {
    // If true then InfluxDB Connection (and ofc Wifi) are established
    if(didConnect)
    {
        selfTrans = 0;
        nextState = requestState;  
    }
    else if(selfTrans < maxSelfTrans)
    {
        selfTrans++;
        nextState = connectState;
    }
    else
    {
        critErrCode = 3; // CritErr, goto ERROR State
    }
  }
}

/*
readSettings: "Passive" Button Handling, do AFTER Influx checkConnection or ssl -1
countTime(STATE_MIN_DUR) && didRequest ? nextState = measureState : nextState = requestState;
*/
void on_requestState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    didRequest = Irrigation::requestData();
    didRequest = Services::readSettings();
  }
  
  if(countTime(STATE_MIN_DUR))
  {
    if(didRequest)
    {
        selfTrans = 0;
        waitTime = 1;
        nextState = measureState;
    }
    else if(selfTrans < maxSelfTrans)
    {     
        selfTrans++;
        // Wait longer than Standard MIN_STATE_DUR
        while(!countTime(waitTime)); 
        waitTime *= 2; // (Seconds)
        nextState = requestState;
    }
    else
    {
        critErrCode = 3;
    }
  }
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    setCpuFrequencyMhz(160);

    // WiFi.disconnect();
#if (DO_MEASURE == 1)
    {
      doMeasurements();
    }
#endif
  }
}

void on_evaluateState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    Irrigation::decidePlants();
  }
}

// Pump, Fan, Heater, ...
// Linked List of ISubStateMachine Pointers, each Machine runs till DONE State
// (Same Pump Obj. runs twice with either SolenoidValve 0 or 1 active, 3.2 and 5.6 Seconds)
void on_actionState()
{
  if (fsm.executeOnce)
  {
    didActions = false;
    commonStateLogic();
  }

#if (RUN_SUBMACHINES == 1)
  {
    // Irrigation::printInstructions(Irrigation::instructions);

    for (auto &instr : Irrigation::irrInstructions)
    {
      Pump *pump = instr.pump;
      // Set back State to idle so that same Pump can run multiple times
      pump->resetMachine();
      // machine->setTime(instr.pumpTime);
      pump->pumpTime = instr.pumpTime;
      pump->relaisChannel = instr.solenoidValve;
      while (!pump->isDone())
      {
        pump->loop();
      }

      // Set ErrorCode for Report
      instr.errorCode = pump->errorCode;

      // Leave Vec unmanipulated (for Report)
      if (&instr == &Irrigation::irrInstructions.back())
      {
        didActions = true;
      }
    }
  }
#endif
}

/*

*/
void on_transmitState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
   
    bool didTramsmit;

    didTransmit = InfluxHelper::writeBuffer();
    
    didTransmit = Irrigation::reportInstructions(Irrigation::pumpInstructions);
    Irrigation::clearInstructions();

    if (!didTransmit)
    {
      if (selfTrans < maxSelfTrans)
      {
        selfTrans++;
        nextState = transmitState;
      }
      else
      {
        critErrCode = 3;
      }
    }
    else
    {
      selfTrans = 0;
    }
  }
}

void on_errorState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    Serial.println(critErrMessage[critErrCode]);
    delay(2000);
    ESP.restart();
  }
}

/*

*/
void checkCriticalErrors()
{
  if (critErrCode != 0)
    // Replaces bugged fsm.transitionTo()
    nextState = errorState;
}

// TRANSITION LOGIC
// IDLE -> INIT
bool transitionS0S1()
{
  if (countTime(STATE_MIN_DUR) && didSleep)
  {
    return true;
  }
  return false;
}

// INIT -> PREP
bool transitionS1S2()
{
  if (countTime(STATE_MIN_DUR))
  {
    return true;
  }
  return false;
}

// PREP-> MEASURE
bool transitionS2S3()
{
  if (countTime(STATE_MIN_DUR) && didConnect)
  {
    return true;
  }
  return false;
}

// MEASURE -> EVALUATE
bool transitionS3S4()
{
  // Wait till max Measure time is up || dht11.didMeasure && aht10.didMeasure && ... (Sensors ready before)
  if (countTime(STATE_MIN_DUR) || climate1.measurementsComplete)
  {
    return true;
  }
  return false;
}

// EVALUATE -> TRANSMIT (No Action needed)
bool transitionS4S6()
{
  if (countTime(STATE_MIN_DUR) && Irrigation::didEvaluate && Irrigation::irrInstructions.size() == 0)
  {
    return true;
  }
  return false;
}

// EVALUATE -> ACTION (Action Stack not empty)
bool transitionS4S5()
{
  if (countTime(STATE_MIN_DUR) && Irrigation::didEvaluate && Irrigation::irrInstructions.size() > 0)
  {
    return true;
  }
  return false;
}

// ACTION -> TRANSMIT (Pop actions until actionStack empty)
bool transitionS5S6()
{
  // min State Duration must be over AND Pump done, Fan done, ...
  if ((countTime(STATE_MIN_DUR) && didActions) || RUN_SUBMACHINES == 0)
  {
    return true;
  }
  return false;
}

// TRANSMIT -> IDLE
bool transitionS6S0()
{
  if (countTime(STATE_MIN_DUR))
  {
    return true;
  }
  return false;
}

/*
// TRANSMIT -> TRANSMIT
bool transitionS6S6()
{
  if (!didTransmit && transmitionAttempts < 2)
  {
    return true;
  }
  return false;
}
*/

void runStateMachine(void *pvParameters)
{
  for (;;)
  {
    if (nextState)
    {
      fsm.transitionTo(nextState);
      nextState = nullptr;
    }
    fsm.run();
    // delay(1);
  }
}

void setup()
{
  // while (!Serial){}
  Serial.begin(115200);

  if (!EEPROM.begin(4096)) // Bytes
  {
    Serial.println("failed to initialise EEPROM");
  }

  // Wire.begin();
  I2Cone.begin(SDA1, SCL1, 150000);
  I2Ctwo.begin(SDA2, SCL2, 100000);

  // Immediately Shut down 2nd ToF so no Init at 0x29
  pinMode(toF_shut, OUTPUT);
  digitalWrite(toF_shut, LOW);

  // Init (LOW-Trigger) Relais
  for (int i = 0; i < 2; i++)
  {
    pinMode(Relais[i], OUTPUT);
    digitalWrite(Relais[i], HIGH);
  }

  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);

  pump1.add_callback(setupToFs);
  pump2.add_callback(setupToFs);

  displayController.setupLEDMatrix();

  Multiplexer::setup();

  idleState = fsm.addState(&on_idleState);
  initState = fsm.addState(&on_initState);
  connectState = fsm.addState(&on_connectState);
  requestState = fsm.addState(&on_requestState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  actionState = fsm.addState(&on_actionState);
  transmitState = fsm.addState(&on_transmitState);
  errorState = fsm.addState(&on_errorState);

  // idleState->addTransition(&transitionS0S1, initState);
  // initState->addTransition(&transitionS1S2, prepState);
  // prepState->addTransition(&transitionS2S3, measureState);
  measureState->addTransition(&transitionS3S4, evaluateState);
  evaluateState->addTransition(&transitionS4S5, actionState);
  evaluateState->addTransition(&transitionS4S6, transmitState);
  actionState->addTransition(&transitionS5S6, transmitState);
  transmitState->addTransition(&transitionS6S0, idleState);

  // https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
  xTaskCreatePinnedToCore(
      runStateMachine, // Function to implement the Task
      "Task2",
      10000,
      NULL,
      0,
      &Task2, // Task handle
      0       // Core running the Task
  );
}

void loop()
{
  // fsm.run();

  Services::webServer.handleClient();

  // displayController.displayPlantStatus();

  ButtonHandler::handleHardwareButtons();

  checkCriticalErrors();

  // delay(100);
}