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

#define SLEEPTYPE 1 // Light Sleep or Modem Sleep
#define LOCALSAVE 1 // Save ArduinoJson Files on Flash
#define DOMEASURE 0
#define SENDDATA 0
#define GETDATA 0
#define RUNSUBMACHINES 0
const int SLEEP_DURATION = 16;
const int MEASURE_INTERVAL = 2;
const int MIN_STATE_DURATION = 2;

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
uint8_t solenoids1[] = {0,1};
uint8_t solenoids2[] = {};
Cistern cistern1(0x51, solenoids1, 300, 53.0f);
Cistern cistern2(0x52, solenoids2, 450, 42.0f);
Pump pump1(0, pump_PWM_1, cistern1);
Pump pump2(1, pump_PWM_2, cistern2);
StatusDisplay displayController;

StateMachine fsm = StateMachine();
State *initState, *sleepState, *measureState, *evaluateState, *actionState, *finishState;
const char *stateNames[] = {"INIT", "SLEEP", "MEASURE", "EVALUATE", "ACTION", "FINISH"};
bool wateringNeeded, didSleep, didCycle;
unsigned long stateBeginMillis = 0;

LinkedList<ISubStateMachine *> actions = LinkedList<ISubStateMachine *>();

void setupToFs()
{
  cistern2.setupToF();
  cistern1.setupToF();
}

// Irrigation.cpp
void refillActions()
{
  // Pumpe 1 shall pump for 2 Plants on 2 different Solenoids consecutively
  // Pump *pump3 = new Pump(1, pump_PWM_2, cistern2);
  Pump *action1 = &pump1;
  Pump *action2 = &pump1;
  actions.add(action1);
  actions.add(action2);
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
  pump1.setupIna();
  lightSensor1.setupTSL2591(I2Cone);
  lightSensor2.setupTSL2591(I2Ctwo);

  // Redo Setup after Wakeup only if necessary
  setupToFs();

  // 3. READ GLOBAL MEASUREMENTS
  if (cistern1.toF_ready)
  {
    cistern1.updateWaterLevel();
  }
  if (cistern2.toF_ready)
  {
    cistern2.updateWaterLevel();
  }

  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // 4. WRITE GLOBAL MEASUREMENTS
  influxHelper.writeDataPoint(p0);

  // PLANT-SPECIFIC MEASUREMENTS
  DynamicJsonDocument moistureSensors = Utilities::readDoc(0, 1024);
  DynamicJsonDocument plants = Utilities::readDoc(3072, 2048);

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

    // For each moistureSensor pinNum assigned to this Plant
    for (int j = 0; j < sizeof(plants[i]["moistureSensors"]); j++)
    {
      int moistureSensor = plants[i]["moistureSensors"][j].as<int>();
      if (moistureSensor != 0) // ignore 0s, e.g. 67000 (=channels 6,7)
      {
        int pinNum = moistureSensor - 1;
        char key[25];
        sprintf(key, "Soil Moisture Sensor %d", moistureSensor);

        int moistureSmoothed = SoilMoisture::measureSoilMoistureSmoothed(pinNum);
        // Pass Reference of moistureSensors Table, so only 1 GET Request instead of per Plant, per Sensor
        int moisturePercentage = SoilMoisture::voltageToPercentage(pinNum, moistureSmoothed, moistureSensors);
        p.addField(key, moisturePercentage);
      }
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
}

void commonStateLogic()
{
  Serial.println(stateNames[fsm.currentState]);
  stateBeginMillis = millis();

  // checkWebButtons();

  // Safety
  pump1.switchOff();
  digitalWrite(Relais[0], HIGH);
  digitalWrite(Relais[1], HIGH);
}

bool countTime(int durationSec)
{
  return (millis() - stateBeginMillis >= durationSec * 1000UL);
}

// STATE LOGIC
void on_initState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    Serial.println("Main State Machine runs on Core: ");
    Serial.println(xPortGetCoreID());

    if (!Services::getWifiStatus())
    {
      Services::setupWifi();
    }

    influxHelper.setParameters();
    if (!influxHelper.checkConnection())
    {
      Serial.println("Couldnt connect to InfluxDB");
    }

#if (GETDATA == 1)
    {
      Utilities::provideData();
    }
#endif

    // WiFi.begin() before this, or Exception
    // Restart necessary after Sleep?
    ButtonHandler::startRestServer();
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

    if (SLEEPTYPE == 0) // Modem Sleep
    {
    }

    else if (SLEEPTYPE == 1) // Light Sleep
    {
      // µs (microseconds)
      esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000 * 1000);
      delay(100); // Else no Wakeup
      esp_light_sleep_start();
      // Resume Program, Connections and States were kept
    }
  }

  if (SLEEPTYPE == -1) // Simulate Sleep
  {
    while (!countTime(SLEEP_DURATION))
    {
    }
  }
  didSleep = true;
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
    Irrigation::decideIrrigation();
    refillActions();
  }
}

// Pump, Fan, Heater, ...
// Stack/Queue/Linked List of action/pump Obj., that calls pop() as soon
// as the FINISH/ABORT State of each Object is reached
// Action State is done when Queue/Linked List is empty
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

      // Set back State to idle so that same Machine can run multiple times
      machine->resetMachine();

      while (!machine->isDone())
      {
        machine->loop();
      }
      actions.remove(i);
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
#if (DOSEND == 1)
    {
      Services::doPostRequest("/commands/ip");
      influxHelper.writeBuffer();
    }
#endif

    didCycle = true;
  }
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
  if (countTime(MIN_STATE_DURATION) && !wateringNeeded) // && !fanNeeded && ...
  {
    return true;
  }
  return false;
}

bool transitionS3S4()
{
  if (countTime(MIN_STATE_DURATION) && wateringNeeded) // || fanNeeded || ...
  {
    return true;
  }
  return false;
}

bool transitionS4S5()
{
  // min State Duration must be over AND Pump done, Fan done, ...
  if ((countTime(MIN_STATE_DURATION) && actions.size() == 0) || RUNSUBMACHINES == 0)
  {
    return true;
  }
  return false;
}

bool transitionS5S2()
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
    fsm.run();
    delay(1);
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

  // Immediately Shut down 2nd ToF, start with different i2C Address later
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

  displayController.setupLEDMatrix();

  Multiplexer::setup();

  initState = fsm.addState(&on_initState);
  sleepState = fsm.addState(&on_sleepState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  actionState = fsm.addState(&on_actionState);
  finishState = fsm.addState(&on_finishState);

  initState->addTransition(&transitionS0S1, sleepState);
  initState->addTransition(&transitionS0S2, measureState);
  sleepState->addTransition(&transitionS1S2, measureState);
  measureState->addTransition(&transitionS2S3, evaluateState);
  evaluateState->addTransition(&transitionS3S4, actionState);
  actionState->addTransition(&transitionS4S5, finishState);
  finishState->addTransition(&transitionS5S2, sleepState);

  // https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
  xTaskCreatePinnedToCore(
      runStateMachine, // Function to implement the task
      "Task2",
      10000,
      NULL,
      0,
      &Task2, // Task handle.
      0       // Core where task runs
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