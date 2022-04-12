#include <main.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <EEPROM.h>
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

TaskHandle_t Task1, Task2;
InfluxHelper influxHelper;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

AmbientClimate climate1(500, 2);
AmbientLight lightSensor1(1);
AmbientLight lightSensor2(2);
// Pumps and associated solenoids/relaisChannels and ToF Sensor never change,
// only associated Pump Model (if User switches Pumps)
uint8_t solenoids1[] = {0, 1};
uint8_t solenoids2[] = {};
Cistern cistern1(0x51, solenoids1, 300, 53.0f);
Cistern cistern2(0x52, solenoids2, 450, 42.0f);
Pump pump1(0, pump_PWM_1, cistern1);
Pump pump2(1, pump_PWM_2, cistern2);
StatusDisplay displayController;
VL53L0X_Error status1 = -1, status2 = -1;

StateMachine fsm = StateMachine();
State *nextState = nullptr;
State *initState, *sleepState, *prepareState, *measureState, *evaluateState, *actionState, *finishState, *errorState;
const char *stateNames[] = {"INIT", "SLEEP", "PREPARE", "MEASURE", "EVALUATE", "ACTION", "FINISH", "ERROR"};
bool didSleep, didConnect, didPrepare, didCycle, toFs_ready;
unsigned long stateBeginMillis = 0;

LinkedList<ISubStateMachine *> actions = LinkedList<ISubStateMachine *>();

bool countTime(int durationSec)
{
  return (millis() - stateBeginMillis >= durationSec * 1000UL);
}

// Always reset/resetup both Sensors at once
void setupToFs()
{
  while(!cistern2.setupToF());
  while(!cistern1.setupToF());
}

void doMeasurements()
{
  Utilities::scanI2CBus(&I2Cone);
  Utilities::scanI2CBus(&I2Ctwo);
  // ESP32 GLOBAL MEASUREMENTS
  // 1. Reuse datapoint
  if (!p0.hasTags())
  {
    p0.addTag("device", DEVICE);
    p0.addTag("SSID", WiFi.SSID());
  }
  p0.clearFields();

  // 2. Setup Sensors
  setupToFs();
  pump1.setupIna();
  lightSensor1.setupTSL2591(I2Cone);
  lightSensor2.setupTSL2591(I2Ctwo);

  // 3. READ GLOBAL MEASUREMENTS
  if(cistern1.toF_ready)
    cistern1.updateWaterLevel();

  if(cistern2.toF_ready)
    cistern2.updateWaterLevel();
  

  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // 4. WRITE GLOBAL MEASUREMENTS
  influxHelper.writeDataPoint(p0);

  // PLANT-SPECIFIC MEASUREMENTS
  DynamicJsonDocument moistureSensors = Utilities::readDoc(0, 1024);
  DynamicJsonDocument plants = Services::doJSONGetRequest("/plants/sensors");
  // DynamicJsonDocument plants = Utilities::readDoc(3072, 2048);

  // 3. Measure (only) assigned Sensors from each plant
  Point p("Plant Data");
  int lastMeasuredLightSensor = -1;
  TSL2591data data;

  // For each plant
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

    // For each moistureSensor pinNum assigned to this Plant
    for (int j = 0; j < array.size(); j++)
    {
      int moistureSensor = array[j];
      int pinNum = moistureSensor - 1;
      char key[25];
      sprintf(key, "Soil Moisture Sensor %d", moistureSensor);

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

    influxHelper.writeDataPoint(p);
  }
  // Serial.printf_P(PSTR("free heap memory: %d\n"), ESP.getFreeHeap());
}

void commonStateLogic()
{
  stateBeginMillis = millis();

  Serial.println();
  Serial.println(stateNames[fsm.currentState]);

  // checkWebButtons();

  // Safety
  pump1.switchOff();
  digitalWrite(Relais[0], HIGH);
  digitalWrite(Relais[1], HIGH);
}

// STATE LOGIC
void on_initState()
{
  if (fsm.executeOnce)
  {
    didConnect = false;
    commonStateLogic();

    Serial.print("Main State Machine runs on Core: ");
    Serial.println(xPortGetCoreID());

    if (!Services::getWifiMultiStatus())
    {
      Services::setupWifiMulti();
    }

    influxHelper.setParameters();
    /*
    if (!influxHelper.checkConnection())
    {
      Serial.println("Couldnt connect to InfluxDB");
    }
    */

    // WiFi.begin() before this, or Exception
    // Restart necessary after Sleep?
    ButtonHandler::startRestServer();
    didConnect = true;
  }
}

/*
https://lastminuteengineers.com/esp32-sleep-modes-power-consumption/
https://m1cr0lab-esp32.github.io/sleep-modes/
1 = Low to High, 0 = High to Low. Pin pulled HIGH
esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);
*/
void on_sleepState()
{
  if (fsm.executeOnce)
  {
    didSleep = false;
    commonStateLogic();
    // ESP.deepSleep(30e6);

    if (SLEEPTYPE == 1) // Light Sleep
    {
      // µs (microseconds)
      esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000 * 1000);
      delay(100); // Else no Wakeup
      esp_light_sleep_start();
      // Resume Program, Connections and States were kept
    }
    else if (SLEEPTYPE == 2)
    {
      // Deep Sleep
    }
  }

  if (SLEEPTYPE == 0) // Simulate Sleep
  {
    while (!countTime(SLEEP_DURATION))
    {
    }
  }
  didSleep = true;
}

void on_prepareState()
{
  if (fsm.executeOnce)
  {
    didPrepare = false;

    commonStateLogic();

    // Send own IP
    Services::doPostRequest("/commands/ip");

    #if (GETDATA == 1)
    {
      Utilities::provideData();
    }
#endif

    didPrepare = true;
  }
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    // WiFi.disconnect();
#if (DOMEASURE == 1)
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
    // Replaces fsm.transitionTo()
    // nextState = errorState;
    Irrigation::decideIrrigation();
  }
}

// Pump, Fan, Heater, ...
// Linked List of ISubStateMachine Pointers, each Machine runs till DONE State
// (Same Pump Obj. runs twice with either SolenoidValve 0 or 1 active, 3.2 and 5.6 Seconds)
void on_actionState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
  }

#if (RUNSUBMACHINES == 1)
  {
    // Run Sub-StateMachines (Pump, Fan, ...) one after another and check if isDone()
    for (int i = 0; i < actions.size(); i++)
    {
      ISubStateMachine *machine = actions.get(i);

      // Set back State to idle so that same Pump can run multiple times
      machine->resetMachine();

      while (!machine->isDone())
      {
        machine->loop();
      }
      // actions.remove(i);
      actions.pop();
    }
  }
#endif
}

void on_finishState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

// Send current IP Address, no Tunneling necessary
#if (SENDDATA == 1)
    {
      influxHelper.writeBuffer();
    }
#endif

    didCycle = true;
  }
}

// Multiple failed Wifi Connects
void on_errorState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    Serial.println("Restarting Device...");
    delay(500);
    ESP.restart();
  }
}

// TRANSITION LOGIC
// INIT -> PREPARE
bool transitionS0S2()
{
  if (countTime(MIN_STATE_DURATION) && didConnect)
  {
    return true;
  }
  return false;
}

// SLEEP -> INIT
bool transitionS1S0()
{
  if (countTime(MIN_STATE_DURATION) && didSleep)
  {
    return true;
  }
  return false;
}

// PREPARE -> MEASURE
bool transitionS2S3()
{
  if (countTime(MIN_STATE_DURATION) && didPrepare)
  {
    return true;
  }
  return false;
}

// MEASURE -> EVALUATE
bool transitionS3S4()
{
  //*
  // Wait till max Measure time is up || dht11.didMeasure && aht10.didMeasure && ... (Sensors ready before)
  if (countTime(MEASURE_INTERVAL) || climate1.measurementsComplete)
  {
    return true;
  }
  return false;
}

// EVALUATE -> FINISH
// No Action needed, go to finishState
bool transitionS4S6()
{
  if (countTime(MIN_STATE_DURATION) && Irrigation::didEvaluate && actions.size() == 0)
  {
    return true;
  }
  return false;
}

// EVALUATE -> ACTION
// Action Stack not empty, go to actionState
bool transitionS4S5()
{
  if (countTime(MIN_STATE_DURATION) && Irrigation::didEvaluate && actions.size() > 0)
  {
    return true;
  }
  return false;
}

// ACTION -> FINISH
// Do/Pop actions until actionStack is empty
bool transitionS5S6()
{
  // min State Duration must be over AND Pump done, Fan done, ...
  if ((countTime(MIN_STATE_DURATION) && actions.size() == 0) || RUNSUBMACHINES == 0)
  {
    return true;
  }
  return false;
}

// FINISH -> SLEEP
bool transitionS6S1()
{
  if (countTime(MIN_STATE_DURATION))
  {
    return true;
  }
  return false;
}

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
  I2Cone.begin(SDA1, SCL1, 200000);
  I2Ctwo.begin(SDA2, SCL2, 100000);

  // Init (LOW-Trigger) Relais
  for (int i = 0; i < 2; i++)
  {
    pinMode(Relais[i], OUTPUT);
    digitalWrite(Relais[i], HIGH);
  }

  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);

  // Immediately Shut down 2nd ToF, start with different i2C Address later
  pinMode(toF_shut, OUTPUT);
  digitalWrite(toF_shut, LOW);

  pump1.add_callback(setupToFs);
  pump2.add_callback(setupToFs);

  displayController.setupLEDMatrix();

  Multiplexer::setup();

  initState = fsm.addState(&on_initState); 
  sleepState = fsm.addState(&on_sleepState);
  prepareState = fsm.addState(&on_prepareState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  actionState = fsm.addState(&on_actionState);
  finishState = fsm.addState(&on_finishState);
  errorState = fsm.addState(&on_errorState);

  initState->addTransition(&transitionS0S2, prepareState);
  sleepState->addTransition(&transitionS1S0, initState);
  prepareState->addTransition(&transitionS2S3, measureState);
  measureState->addTransition(&transitionS3S4, evaluateState);
  evaluateState->addTransition(&transitionS4S5, actionState);
  evaluateState->addTransition(&transitionS4S6, finishState);
  actionState->addTransition(&transitionS5S6, finishState);
  finishState->addTransition(&transitionS6S1, sleepState);

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

  ButtonHandler::webServer.handleClient();

  // displayController.displayPlantStatus();

  ButtonHandler::handleHardwareButtons();

  // delay(100);
}