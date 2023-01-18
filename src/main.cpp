#include <main.h>
#include <LinkedList.h>
#include <StateMachine.h> // Quelle: https://github.com/jrullan/StateMachine
#include <ArduinoJson.h>
#include <Irrigation.h>
#include <Utilities.h>
#include <AmbientClimate.h>
#include <SoilMoisture.h>
#include <AmbientLight.h>
// #include <FlowMeter.h>
// #include <PowerMeter.h>
#include <StatusDisplay.h>
#include <ButtonHandler.h>
#include <Pump.h>
#include <LinkedList.h>
#include <ISubStateMachine.h>
#include <Evaluation.h>
#include <math.h>
// #include <Settings.h>

static Utilities utils;
static Evaluation eval;

const char baseUrl[] = "https://juli.uber.space/node";
uint16_t WATER_LIMIT_2h = 99999, WATER_LIMIT_24h = 99999; // 1000, 4000 ml
uint8_t IDLE_DUR = 4, SLEEP_TYPE = 0, SLEEP_DUR = 16, STATE_MIN_DUR = 4;
float PUMP_TIME_LIMIT = 60.0f;
uint32_t stateBeginMillis = 0;

TaskHandle_t FSM_Main_Task; // runs hierarchical StateMachine
TwoWire I2Cone = TwoWire(0), I2Ctwo = TwoWire(1);

PowerMeter powerMeter1(I2Ctwo);
AmbientClimate climate1(2.0f, dhtInPin); // DHT22
AmbientLight lightSensor1(1), lightSensor2(2);
StatusDisplay displayController;

// uint8_t solenoids1[] = {1, 2}, solenoids2[] = {};
// FlowMeter flowMeter1(flowPin);
Cistern cistern2(toF_Adresses[1]); // cistern1(0x51, flowMeter1)
Pump pump1(cistern2); // pump2(flowMeter1, cistern2);

StateMachine fsm = StateMachine();
State *currentState = nullptr, *nextState = nullptr, *referralState = nullptr;
State *idleState, *initState, *connectState, *requestState, *measureState, *evaluateState, *actionState, *transmitState, *errorState;

char *critErrMessage = nullptr;
uint8_t critErrCode = 0;
const char *const critErrMessages[] = {"No Error", "Final Fail to connect WiFi", "Final Fail to connect to InfluxDB", "Final Fail to setup ToFs"};


// Use Indexes instead; Use Vector?
LinkedList<String> manualTransitions = LinkedList<String>();
// std::vector<String> transDestinations = {};

State *getStateByName(String stateName)
{
  State *state = nullptr;

  for (int i = 0; i < fsm.stateList->size(); i++)
  {
    state = fsm.stateList->get(i);

    if (stateName.equals(state->name))
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
  for (int i = 1; i < fsm.stateList->size() - 1; i++)
  {
    State *state = fsm.stateList->get(i);
    manualTransitions.add(state->name);
  }
}

// Always setup both Sensors at once, use SHUT-Pin
void setupToFs()
{
  cistern2.setupToF();
  // while (!cistern2.setupToF());
  // while (!cistern1.setupToF());
}

/*
Unique Device ID (MAC-Addr.), add Tag to all Measurements
-> Grafana Panels change content depending on device Variable
*/
void getDeviceID()
{
  uint32_t macAddr = ESP.getEfuseMac() & 0xFFFFFFFF;
  String device_id = String(macAddr);
  // Serial.println(device_id);
  // p0.addTag("device", device_id);

  byte mac[6];
  WiFi.macAddress(mac);
  String deviceID = String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);
  Serial.println(deviceID);
}

bool measureSensors()
{
  p0.clearTags();
  p0.clearFields();

  // ENVIRONMENT_DATA
  p0.addTag("device", "O217_1"); // DEVICE
  // p0.addTag("SSID", WiFi.SSID());
  int8_t rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // Measure AmbientClimate (or contact SensorBox)

  // Measure all Cisterns
  if (cistern2.toF_ready() == true)
  {
    uint16_t currLiquidVolume = cistern2.getLiquidAmount();
    cistern2.updateLiquidAmount();
  }

  // PLANT_DATA
  // Advantage: Local DynamicJsonDocs (big) get destroyed after leaving Scope
  DynamicJsonDocument plants(2048);
  Services::doJSONGetRequest("/plants", plants);

  Point p("Plant Data");
  
  LightData data;
  data.infraRed = 0;
  data.visibleLight = 0;
  int lastMeasuredLightSensor = -1;

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

    // Measure soilSensors assigned to Plant
    JsonArray sensors = plants[i]["moistureSensors"];
    SoilMoisture::measurePlant(sensors, p);

    /* Measure lightSensor assigned to Plant
    Sort MongoDb Cursor by lightSensor
    (Many plants will have the same sensor, dont measure for each)
    */
    int lightSensor = plants[i]["lightSensor"].as<int>();

    if (lightSensor != 0) // .isNull() not possible for int
    {
      if (lightSensor != lastMeasuredLightSensor)
      {
        data = lightSensor2.measure();
        lastMeasuredLightSensor = lightSensor;
      }

      // Only add if sensor assigned / data read, otherwise random values in field
      p.addField("infraredLight", data.infraRed);
      p.addField("visibleLight", data.visibleLight);
    }

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
    if (manualTransitions.get(0).equals(currentState->name) && SLEEP_TYPE == 0)
    {
      manualTransitions.remove(0);
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

  if (SLEEP_TYPE != 0)
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
Only do when in manual Idle Mode
*/

bool transitionToTarget()
{
  // if (SLEEP_TYPE == 0)
  {
    if (currentState->didActivities)
    {
      // printDestinations();

      if (manualTransitions.size() > 0)
      {
        State *target = getStateByName(manualTransitions.get(0));
        manualTransitions.remove(0);

        if (target != nullptr)
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
      currentState->transCount++;
      // while(!Utilities::countTime(currentState->beginTime, currentState->minStateTime*currentState->selfTrans));
      nextState = currentState;
    }
    else
    {
      // critErrMessage = currentState->name;
      if (currentState == idleState)
        currentState->transCount = 0;

      nextState = errorState;
    }
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
Keep on transitioning to Self, no blocking while needed
Go to REQUEST State periodically (Polling) to check if User added Actions / changed Settings
*/
void on_idleState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();

    if (SLEEP_TYPE == 0) // No Sleep / DemoMode
    {
      // doPolling
      int pollingInterval = 5;
      if (currentState->transCount % pollingInterval == 0)
      {
        manualTransitions.add("REQUEST");
      } 

      // doPolling, stop idling, OR:
      // User has set a manual transitionTarget, stop idling
      currentState->didActivities = manualTransitions.size() > 0;
    }
    else if (SLEEP_TYPE == 1) // Light Sleep
    {
      Serial.println("Entering Light Sleep.");
      esp_sleep_enable_timer_wakeup(SLEEP_DUR * 1000 * 1000); // s to µs
      delay(100);                                             // else no Wakeup
      esp_err_t sleepError = esp_light_sleep_start();

      /*
      Resume Program, Wifi and InfluxDB Connections were kept
      ("Connected to InfluxDB")
      -> Skip INIT and CONNECT?
      */
      if(sleepError == ESP_OK)
      {
        Serial.println("Sleep succeeded");
        // transDestinations.add("REQUEST");
        currentState->didActivities = true;
      }
        
    }
    else if (SLEEP_TYPE == 2)
    {
      // ESP.deepSleep(30e6);
    }
  }

  // Check constantly after minStateTime is up
  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    if (transitionToTarget())
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

    Serial.print("Device MAC-Address: ");
    getDeviceID();

    Serial.print("Main State Machine runs on Core: ");
    Serial.println(xPortGetCoreID());

    // Setup ToF Sensors
    setupToFs();

    powerMeter1.setup();

    // climate1.setup();

    lightSensor1.setup(I2Cone);
    lightSensor2.setup(I2Ctwo);
  }

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    Utilities::scanI2CBus(&I2Cone);
    Utilities::scanI2CBus(&I2Ctwo);

    // Start checking bool after minStateTime, Give Sensors time to init
    initState->didActivities = powerMeter1.isReady();
    initState->didActivities = lightSensor2.isReady();
    // climate1.printInfo();

    if (transitionToTarget())
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
    connectState->didActivities = InfluxHelper::connectionEstablished();

    // Connect to SensorBox
    // connectState->didActivities = connectToBox();
  }

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    if (transitionToTarget())
      return;
    if (transitionToSelf())
      return;
    transitionToNext();
  }
}

/*
Request needed Data from APIs (REST-Server, Pl@ntNet, SensorBox, ...)
Get settings mongoDB-doc (usersettings, actions)
*/
void on_requestState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    /*
    Do one entry Func after another, if one fails do immediate selfTransition
    Choose which one is critical to repeat or not (dont include in if statement)
    */
    // Irrigation::getRecentIrrigations(); not critical, one try
    // !Irrigation::updateWaterDistribution() || 
    if(!Services::readSettings())
      transitionToSelf();
    currentState->didActivities = true;
  }

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    // Serial.println("Manual Transitions: ");
    // printDestinations();

    if (transitionToTarget())
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
      measureState->didActivities = measureSensors();
    }
#endif
  }

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    if (transitionToTarget())
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
    evaluateState->didActivities = eval.evaluatePlants();
  }

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    if (transitionToTarget())
      return;
    if (transitionToSelf())
      return;
    transitionToNext();

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

    if (Irrigation::instructions.size() == 0)
    {
      Serial.println("No scheduled automatic or manual Irrigations.");
      actionState->didActivities = true;
    }
  }

#if (RUN_SUBMACHINES == 1)
  {
    if (Irrigation::instructions.size() > 0)
    {
      for (auto &instr : Irrigation::instructions)
      {
        // runInstruction (auslagern)

        // Always use same Obj. for Submachine, or make static
        // Pump *pump = instr.pump;
        Pump *pump = &pump1;

        if (instr.errorCode == 0) // Only run valid Instructions
        {
          // Reset pump Obj. on each run
          pump->resetMachine();

          // Needed Infos for StateMachine Run
          pump->instr = &instr;
          // pump->pumpTime = instr.pumpTime;
          // pump->cistern.maxPossibleDist = 5;
          // pump->allocatedWater = instr.allocatedWater;
          // pump->relaisChannel = instr.solenoidValve;

          while (!pump->machineDone())
          {
            pump->loop();
          }

          // TODO Multiple errorCodes?
          instr.errorCode = pump->errorCode;

          // instr.distributedWater = pump->cistern.pumpedLiquid;
        }
        else
        {
          // instr.distributedWater = 0;

          Serial.println("Action aborted.");
          // Serial.println(Irrigation::errors[instr.errorCode]);
          Irrigation::printError(instr.errorCode);
        }

        /*    
        Write Point after Executing), not all at once 
        (-> correct Timestamp, better Visibility in Grafana)
        Write Points into Buffer (->no Delay Executing Actions)
        */
        Irrigation::reportInstruction(instr);

        if (&instr == &Irrigation::instructions.back())
        {
          // Clear Vector
          Irrigation::clearInstructions();
          actionState->didActivities = true;
        }
      }
    }
  }
#endif

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    /*
    // Go back to EVALUATE if instructions were manual
    if (referralState == requestState)
    {
      nextState = evaluateState;
      return;
    }
    */

    if (transitionToTarget())
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

  if (utils.countTime(stateBeginMillis, currentState->minStateTime))
  {
    if (transitionToTarget())
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
  }

  if(utils.countTime(stateBeginMillis, currentState->minStateTime))
    ESP.restart();
}

/*
Infinite Loop, run FSM
nextState cause Library transitionTo doesn't work
https://github.com/jrullan/StateMachine/issues/13
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

/*
attachInterrupt needs Class method / Function without hidden this param (Member method)
*/
void onInterrupt_1()
{
  pump1.flow->pulse();
  // flowMeter1.pulse();
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
    pinMode(relaisPins[i], OUTPUT);
    digitalWrite(relaisPins[i], HIGH);
  }

  pinMode(dhtInPin, INPUT);
  pinMode(flowPin1, INPUT);
  // attachInterrupt(GPIOpin, ISR, Event);
  attachInterrupt(flowPin1, onInterrupt_1, RISING);

  pump1.add_callback(setupToFs);

  displayController.setupLEDMatrix();

  Multiplexer::setupPins();

  idleState = fsm.addState(&on_idleState);
  initState = fsm.addState(&on_initState);
  connectState = fsm.addState(&on_connectState);
  requestState = fsm.addState(&on_requestState);
  measureState = fsm.addState(&on_measureState);
  evaluateState = fsm.addState(&on_evaluateState);
  actionState = fsm.addState(&on_actionState);
  transmitState = fsm.addState(&on_transmitState);
  errorState = fsm.addState(&on_errorState);

  // Write Constructor for State?
  idleState->name = "IDLE";
  initState->name = "INIT";
  connectState->name = "CONNECT";
  requestState->name = "REQUEST";
  measureState->name = "MEASURE";
  evaluateState->name = "EVALUATE";
  actionState->name = "ACTION";
  transmitState->name = "TRANSMIT";
  errorState->name = "ERROR";
  // Set different Parameters than Standard
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

  // Do one Cycle first in demoMode to setup sensors
  if (SLEEP_TYPE == 0)
  {
    addCycle();
    Utilities::printDestinations();
  }

  // https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
  // Run on Core 1, since Core 0 handles TCP, I2C, Kernel, ...
  xTaskCreatePinnedToCore(
      runStateMachine, // Function to implement the Task
      "FSM_Main_Task",
      10000,
      NULL,
      0,
      &FSM_Main_Task, // Task handle
      1       // Core running the Task
  );

  /*
  Disable Brownout Warnings (Occurs when on Usb...)
  then instead: ClearCommError failed, Crash
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  */
}

// Executed on CPU Core 1
void loop()
{
  // fsm.run();

  Services::webServer.handleClient();

  // displayController.displayPlantStatus();

  ButtonHandler::handleHardwareButtons();

  // checkCriticalErrors();

  // delay(100);
}