// #include <PowerMeter.h>
#include <main.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <ArduinoJson.h>
#include <Irrigation.h>
#include <Utilities.h>
#include <AmbientClimate.h>
#include <SoilMoisture.h>
#include <AmbientLight.h>
#include <FlowMeter.h>
#include <StatusDisplay.h>
#include <ButtonHandler.h>
#include <Pump.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>
#include <math.h>

const char baseUrl[] = "https://juli.uber.space/node";
uint8_t IDLE_DUR = 4, SLEEP_DUR = 16, STATE_MIN_DUR = 4;
uint32_t stateBeginMillis = 0;

TaskHandle_t Task1, Task2;
TwoWire I2Cone = TwoWire(0), I2Ctwo = TwoWire(1);

PowerMeter powerMeter1(I2Ctwo);
AmbientClimate climate1(2.0f, dhtInPin); // DHT22
AmbientLight lightSensor1(1), lightSensor2(2);
FlowMeter flowMeter1(flowPin);
StatusDisplay displayController;

// uint8_t solenoids1[] = {1, 2}, solenoids2[] = {};
Cistern cistern2(0x52, flowMeter1); // cistern1(0x51, meter1)
Pump pump1(0, pump_PWM_1, cistern2), pump2(1, pump_PWM_2, cistern2);

StateMachine fsm = StateMachine();
State *currentState = nullptr, *nextState = nullptr, *referralState = nullptr;
State *idleState, *initState, *connectState, *requestState, *measureState, *evaluateState, *actionState, *transmitState, *errorState;
uint8_t critErrCode = 0;
char *critErrMessage = "";
// const char *critErrMessage[] = {"None", "Final Fail to connect WiFi", "Final Fail to connect to InfluxDB", "Final Fail to setup ToFs"};
// const char *stateNames[] = {"IDLE", "INIT", "CONNECT", "REQUEST", "MEASURE", "EVALUATE", "ACTION", "TRANSMIT", "ERROR"};

// Efficient remove at start, add at end, no random access; Why not vector?
LinkedList<String> transDestinations = LinkedList<String>(); // "Eingabewörter"
// std::vector<String> transDestinations = {};

bool countTime(int durationSec)
{
  return (millis() - stateBeginMillis >= durationSec * 1000UL);
}

State* getStateByName(String stateName)
{
  State* state = nullptr;

  for(int i = 0; i < fsm.stateList->size()-1; i++)
    {
      state = fsm.stateList->get(i);

      if(stateName.equals(state->name))
      {
        return state;
      }
    }

  return state;
}

/*
Do one whole Cycle in Demo Mode first
Skip IDLE and ERROR
*/
void addCycle()
{ 
  for(int i = 1; i < fsm.stateList->size()-1; i++)
  {
    State* state = fsm.stateList->get(i);
    transDestinations.add(state->name);
  }
}

// Always setup both Sensors at once
void setupToFs()
{
  while (!cistern2.setupToF());
  // while (!cistern1.setupToF());
}

bool doMeasurements()
{
  // ESP32 GLOBAL MEASUREMENTS
  if (!p0.hasTags()) // Reuse Datapoint
  {
    p0.addTag("device", DEVICE);
    p0.addTag("SSID", WiFi.SSID());
  }
  p0.clearFields();

  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // Measure Climate
  Serial.println(climate1.measureTemperatureDHT());
  Serial.println(climate1.measureHumidityDHT());

  // Measure Water
  if (cistern2.toF_ready)
    //cistern2.updateWaterLevel();
    cistern2.waterManagement();

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
      snprintf(key, 64, "soilMoisture%d", moistureSensor);

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
    p.addField("infraredLight", data.infraRed);
    p.addField("visibleLight", data.visibleLight);

    InfluxHelper::writeDataPoint(p);
  }
  return true;
}

/*
Logic at StateBegin, evaluated only once
Do Stuff like check Connection/Sensors, transition back to State if needed
Clock down in CONNECT and TRANSMIT States
*/
void commonStateLogic()
{
  stateBeginMillis = millis();
  Utilities::stateBeginMillis = millis();

  // setCpuFrequencyMhz(currentState->clockSpeed);

  currentState = fsm.stateList->get(fsm.currentState);
  currentState->didActivities = false; // "Reset" States
  currentState->transCount++;

  // Serial.println(stateNames[fsm.currentState]);
  Serial.println();
  Serial.println(currentState->name);

  /*
  // Safety
  pump1.switchOff();
  pump2.switchOff();
  digitalWrite(Relais[0], HIGH);
  digitalWrite(Relais[1], HIGH);
  */
}

/*
State Index of manual Transition reached, go Back to Idle
Evaluate this Transition first, if State has it
Make as extra Trans so not every State has it
*/
bool transitionToIdle()
{
  if (currentState->didActivities)
  {
    if (transDestinations.get(0).equals(currentState->name) && SLEEPTYPE == 0)
    {
      transDestinations.remove(0);
      // printDestinations();
      nextState = idleState;
      return true;
    }
  }
  return false;
}

/*
Generalized Transition Funcs, evaluated continously
Evaluation Order important
Return true/false, to reliably stop evaluation of next transitions
(...When a state has multiple transitions, they are evaluated in the order they were added to the state)
If manualTransition == currentStateIndex and didSucceed, go back to Idle
*/
bool transitionToNext()
{
  uint8_t current = fsm.currentState;
  // Starts from 0, and dont include last State (errorState), hence -2
  uint8_t next = current < fsm.stateList->size() - 2 ? ++current : 0;

  if(SLEEPTYPE != 0)
  {
    if (currentState->didActivities)
    {
      currentState->transCount = 0;
      nextState = fsm.stateList->get(next);
      return true;   
    }
  }
  
  return false;
}

/*
Directly go to User targetState, only go (back) to
other States if necessary (Connection lost etc.)
Call whenever transDestinations.size > 0
*/

bool transitionToTarget()
{
  int index;

  if (currentState->didActivities)
  {
    // printDestinations();

    if(transDestinations.size() > 0)
    {
      State* target = getStateByName(transDestinations.get(0));
      transDestinations.remove(0);
  
      if(target != nullptr)
      {
        currentState->transCount = 0;
        nextState = target;
      }  
    }
    else
    {
      // Serial.println("All manual Transitions done.");
      nextState = idleState;
    }   
  }
  
  return false;
}

/*
Increase waitTime?
Idle State can repeat indefinately
*/
bool transitionToSelf()
{
  if (!currentState->didActivities)
  {
    if (currentState->transCount < currentState->maxSelfTrans)
    {
      // currentState->transCount++;
      // while(!Utilities::countTime(currentState->beginTime, currentState->minStateTime*currentState->selfTrans));
      nextState = currentState;
    }
    else
    {
      // critErrMessage = currentState->name;
      if(currentState == idleState)
        currentState->transCount = 0;

      nextState = errorState;
    }
    return true;
  }
  return false;
}

/*
Check Settings / User Actions every n (interval) idle Transitions
Set transCount to 0, or maxSelfTrans is reached after returning to IDLE, -> ERROR State
*/
bool checkSettings()
{
  uint8_t interval = 5;
  // Serial.println(currentState->transCount);

  if (currentState->transCount % interval == 0)
  {
    transDestinations.add("REQUEST");
    transitionToTarget();
    /*
    currentState->transCount = 0;
    referralState = currentState;
    nextState = connectState;
    */
    return true;
  }
  return false;
}

/*
https://lastminuteengineers.com/esp32-sleep-modes-power-consumption/
https://m1cr0lab-esp32.github.io/sleep-modes/
1 = Low to High, 0 = High to Low. Pin pulled HIGH
esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);

SLEEPTYPE == 0
Simulate Sleep/Idle
Keep on transitioning to Self, no blocking while or countTime needed
Go to CONNECT,REQUEST State periodically to check if User set manualTransition
*/
void on_idleState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    if (SLEEPTYPE == 1) // Light Sleep
    {
      Serial.println("Entering Light Sleep.");
      esp_sleep_enable_timer_wakeup(SLEEP_DUR * 1000 * 1000); // µs
      delay(100);                                             // else no Wakeup
      esp_light_sleep_start();
      // Resume Program, Connections and States were kept, go to INIT
      currentState->didActivities = true;
    }
    else if (SLEEPTYPE == 2)
    {
      // ESP.deepSleep(30e6);
    }
    else if (SLEEPTYPE == 0) // No Sleep / Demonstration Mode
    {
      // User has set a manual transitionTarget, stop idling
      // currentState->didActivities = transDestinations.size() > 0;
    }
  }

  if (countTime(currentState->minStateTime))
  {
    // ESP.deepSleep done or idling time (demo mode) up
    currentState->didActivities = true;

    /*
    // Do whole Cycle once after Bootup
    if (!initState->didActivities)
    {
      nextState = initState;
      return;
    }
    */

    if (checkSettings())
      return;
    if(transitionToTarget());
      return; 
    if (transitionToSelf())
      return;

    transitionToNext();
  }
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

    Serial.print("Main State Machine runs on Core: ");
    Serial.println(xPortGetCoreID());

    // Setup Sensors
    setupToFs();

    powerMeter1.setupIna();

    climate1.setup();

    lightSensor1.setupTSL2591(I2Cone);
    lightSensor2.setupTSL2591(I2Ctwo);
    // sensorsReady = lightSensor1.isReady();

    // Serial.println("I2C Bus 1:");
    Utilities::scanI2CBus(&I2Cone);
    // Serial.println("I2C Bus 2:");
    Utilities::scanI2CBus(&I2Ctwo);
  }

  if (countTime(currentState->minStateTime))
  {
    // Start checking bool after minStateTime, Give Sensors time to init
    initState->didActivities = powerMeter1.inaReady();
    initState->didActivities = lightSensor2.isReady();
    climate1.printInfo();

    if(transitionToTarget());
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
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
Don't set Params if Conn already exsists
*/
void on_connectState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    while (!Services::wifiConnected())
      Services::setupWifi();

    // Set before establishing any Connection to InfluxDB

    while (!InfluxHelper::setParameters())
      ;

    // If true then InfluxDB Connection (and ofc Wifi) are established
    connectState->didActivities = InfluxHelper::checkConnection();

    // Connect to SensorBox
    // connectState->didActivities = connectToBox();
  }

  if (countTime(currentState->minStateTime))
  {
    if(transitionToTarget());
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
  }
}

/*
readSettings: "Passive" Button Handling, do AFTER Influx checkConnection or ssl -1
countTime(STATE_MIN_DUR) && didRequest ? nextState = measureState : nextState = requestState;
InfluxDB Connection is held open, probably not many recent Irrigations -> updateRecentIrrigations not so expensive
*/
void on_requestState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    requestState->didActivities = Irrigation::updateRecentIrrigations();
    requestState->didActivities = Services::readSettings();
  }

  if (countTime(currentState->minStateTime))
  {
    // Serial.println("User Actions: ");
    //printDestinations();

    if(transitionToTarget());
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
  }
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    // WiFi.disconnect();
#if (DO_MEASURE == 1)
    {
      measureState->didActivities = doMeasurements();
    }
#endif
  }

  if (countTime(currentState->minStateTime))
  {
    if(transitionToTarget());
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
  }
}

void on_evaluateState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    evaluateState->didActivities = Irrigation::evaluatePlants();
  }

  if (countTime(currentState->minStateTime))
  {
    if(transitionToTarget());
      return;

    /*
    if (currentState->didActivities)
    {
      
      // "Custom" Transition
      if (Irrigation::instructions.size() > 0)
      {
        nextState = actionState;
      }
      else
      {
        nextState = transmitState;
      }  
    }
    */
  }
}

/*
Pump, Fan, Heater, ...
For each Instruction, run SubStateMachine until it reaches DONE State
*/
void on_actionState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    if(!Irrigation::instructions.size() > 0)
    {
      Serial.println("No scheduled automatic or manual Irrigations.");
      actionState->didActivities = true;
    }
  }

  #if (RUN_SUBMACHINES == 1)
  {
    if(Irrigation::instructions.size() > 0)
    {
      for (auto &instr : Irrigation::instructions)
      {
        Pump *pump = instr.pump;

        if (instr.errorCode == 0) // Only run valid Instructions
        {
          // Set back State to idle so that same Pump can run multiple times
          pump->resetMachine();
    
          // Infos set before StateMachine Run
          pump->pumpTime = instr.pumpTime;

          pump->relaisChannel = instr.solenoidValve;

          while (!pump->isDone())
          {
            pump->loop();
          }

          // Only set this is if pump ran into ABORT, 
          // if there is an error before (Irrigation, Plant/Pump not found etc.) then don't override that errorCode
          instr.errorCode = pump->errorCode;

          instr.distributedWater = pump->cistern.pumpedWater;
        }
        else
        {
          instr.distributedWater = 0;

          Serial.println("Action aborted.");
          // Serial.println(Irrigation::errors[instr.errorCode]);
          Irrigation::printError(instr.errorCode);
        }

        // Write Points into Buffer (no Delay), correct Timestamp (Order of Pumping Processes)
        // and better readable Points in Grafana Graph
        Irrigation::reportInstruction(instr);

        if (&instr == &Irrigation::instructions.back())
        {
          // create/write/run manual Instr, write Reports, clear Vec, return to EVALUATE, then
          // decidePlants (based on new Data), create/write automatic Instructions
          // Irrigation::reportToMongo(Irrigation::instructions);
          // Irrigation::reportInstructions(Irrigation::instructions);
          Irrigation::clearInstructions();
          actionState->didActivities = true;
        }  
      }
    } 
  }
#endif

  if (countTime(currentState->minStateTime))
  {
    /*
    // Go back to EVALUATE if instructions were manual
    if (referralState == requestState)
    {
      nextState = evaluateState;
      return;
    }
    */

    if(transitionToTarget());
      return;
    transitionToNext();
  }
}

void on_transmitState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    // Write reports before clearing Buffer!
    // transmitState->didActivities = Irrigation::reportInstructions(Irrigation::instructions);
    transmitState->didActivities = InfluxHelper::writeBuffer();
  }

  if (countTime(currentState->minStateTime))
  {
    if(transitionToTarget());
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
  }
}

void on_errorState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    Serial.println(critErrMessage[critErrCode]);
    while (!countTime(currentState->minStateTime))
      ;
    ESP.restart();
  }
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

/*
Attach meter1 Member method (hidden this Param) to Interrupt on Pin ...
attachInterrupt needs Class method / Function without this param
*/
void onInterrupt_1()
{
  flowMeter1.pulse();
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

  pinMode(dhtInPin, INPUT);
  pinMode(flowPin, INPUT);
  attachInterrupt(flowPin, onInterrupt_1, RISING);

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

  // Write another Constructor for State?
  idleState->name = "IDLE";
  initState->name = "INIT";
  connectState->name = "CONNECT";
  requestState->name = "REQUEST";
  measureState->name = "MEASURE";
  evaluateState->name = "EVALUATE";
  actionState->name = "ACTION";
  transmitState->name = "TRANSMIT";
  errorState->name = "ERROR";
  initState->minStateTime = 6;
  idleState->maxSelfTrans = 6; // 1 + interval (checkSettings)

  /*
  // States added like this always get evaluated first
  // Additional Transitions directly defined in on_idle etc. get evaluated last -> problematic
  idleState->addTransition(&checkSettings, connectState);
  for(int i = 0; i < fsm.stateList->size(); i++)
  {
    if(i < 3)
    {
      State *cState = fsm.stateList->get(i);
      State *nState = fsm.stateList->get(i+1);
      fsm.stateList->get(i)->addTransition(&transitionToSelf, cState);
      fsm.stateList->get(i)->addTransition(&transitionToNext, nState);
    }
  }
  */

  // Begin here
  currentState = idleState;

  if(SLEEPTYPE == 0)
  {
    addCycle();
    Utilities::printDestinations();
  }

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

  /*
  Disable Brownout Warnings (Occurs when on Usb...)
  then instead: ClearCommError failed, Crash
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  */
}

void loop()
{
  // fsm.run();

  Services::webServer.handleClient();

  // displayController.displayPlantStatus();

  ButtonHandler::handleHardwareButtons();

  // checkCriticalErrors();

  // delay(100);
}