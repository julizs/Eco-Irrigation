#include <main.h>
#include <AmbientClimate.h>
#include <SoilMoisture.h>
#include <AmbientLight.h>
#include <Pump.h>
#include <StatusDisplay.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <ArduinoJson.h>
#include <ButtonHandler.h>

const char baseUrl[] = "https://juli.uber.space/node";

TaskHandle_t Task1, Task2;

Services services;
ButtonHandler buttonHandler;
InfluxHelper influxHelper;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

// Fotoresistor fotoResistor1(10000, 3.3, 10, C15);
AmbientClimate climate1(500, 2);
AmbientLight lightSensor1(1);
AmbientLight lightSensor2(2);
Cistern cistern1(0x51, 300, 53.0f); // shut
Cistern cistern2(0x52, 350, 42.0f);
Pump pump1(0, pump_PWM_1, cistern1);
Pump pump2(1, pump_PWM_2, cistern2);
StatusDisplay displayController;

// Plant plant1(lightSensor2, soilMoisture1);
// Plant plant2(lightSensor2, soilMoisture2);
// std::vector<Plant> plants{plant1, plant2};

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
  Serial.println("Scanning I2C Addresses Channel: ");
  uint8_t cnt = 0;
  for (uint8_t i = 0; i < 128; i++)
  {
    wire->beginTransmission(i);
    uint8_t ec = wire->endTransmission(true);
    if (ec == 0)
    {
      if (i < 16)
        Serial.print('0');
      Serial.print(i, HEX);
      cnt++;
    }
    else
      Serial.print("..");
    Serial.print(' ');
    if ((i & 0x0f) == 0x0f)
      Serial.println();
  }
  Serial.print("Scan Completed, ");
  Serial.print(cnt);
  Serial.println(" I2C Devices found.");
}

/*
void setupToFs()
{
  int attempt = 0;
  // Setup ToF in correct Order
  while (!cistern2.toF_ready && attempt < 4)
  {
    cistern2.setupToF();
    attempt++;
    delay(1000);
  }
  while (!cistern1.toF_ready && attempt < 4)
  { // 0x51
    // cistern1.shutToF();
    // delay(100);
    cistern1.setupToF();
    attempt++;
    delay(1000);
  }
}
*/

void doMeasurements()
{
  scanI2CBus(&I2Cone);
  scanI2CBus(&I2Ctwo);

  // ESP32 (Global/Per Device) Measurements
  // Reuse datapoint, add tags and clear fields
  if (!p0.hasTags())
  {
    p0.addTag("device", DEVICE);
    p0.addTag("SSID", WiFi.SSID());
  }

  p0.clearFields();

  // Redo setup after Wakeup if necessary
  while(!cistern2.setupToF()) {}
  while(!cistern1.setupToF()) {}
  // setupToFs();

  // Skip? WaterLevel only changes if Pump was active
  if (cistern1.toF.Status == 0)
  {
    int waterLevel1 = cistern1.evaluateToF();
    p0.addField("water level bucket 1", waterLevel1);
  }
  if (cistern2.toF.Status == 0)
  {
    int waterLevel2 = cistern2.evaluateToF();
    p0.addField("water level bucket 2", waterLevel2);
  }

  // Read RSSI
  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // Idle Power Consumption of L298N
  INAdata inaData = pump1.readIna();
  p0.addField("voltage", inaData.voltage);
  p0.addField("current", inaData.current);
  p0.addField("power", inaData.power);
  p0.addField("busVoltage", inaData.busVoltage);
  p0.addField("shuntVoltage", inaData.shuntVoltage);

  pump1.setupIna();

  influxHelper.writeDataPoint(p0);

  // PLANT-SPECIFIC MEASUREMENTS
  // 1. Get User-assigned Plant-Sensor Assignments
  char url[50] = "";
  strcat(url, baseUrl);
  strcat(url, "/plants/json");
  DynamicJsonDocument doc = services.doJSONGetRequest(url);

  // 2. Setup Sensors only once
  lightSensor1.setupTSL2591(I2Cone);
  lightSensor2.setupTSL2591(I2Ctwo);

  // 3. Measure (only) assigned Sensors from each plant
  Point p("Plant Data"); // Write new data point/row into this table, reuse datapoint
  int lastMeasuredLightSensor = -1;
  TSL2591data data;

  // For each plant
  for (int i = 0; i < doc.size(); i++)
  {
    String plantName = doc[i]["name"].as<String>();
    p.clearTags();
    p.clearFields();
    p.addTag("Plant", plantName);

    Serial.print("Plant: ");
    Serial.println(doc[i]["name"].as<String>());

    for (int j = 0; j < sizeof(doc[i]["moistureSensors"]); j++)
    {
      int moistureSensor = doc[i]["moistureSensors"][j].as<int>();
      if (moistureSensor != 0) // ignore 0s, e.g. 67000 (=channels 6,7)
      {
        int pinNum = moistureSensor - 1;
        char key[25];
        sprintf(key, "Soil Moisture Sensor %d", moistureSensor);

        int moistureSmoothed = SoilMoisture::measureSoilMoistureSmoothed(pinNum);
        int moisturePercentage = SoilMoisture::voltageToPercentage(pinNum, moistureSmoothed);
        p.addField(key, moisturePercentage);
      }
    }

    // MongoDb Cursor gets sorted by lightSensor, same Sensor measures less often
    int lightSensor = doc[i]["lightSensor"].as<int>();

    if (lightSensor != lastMeasuredLightSensor)
    {
      data = lightSensor2.measureLight();
      lastMeasuredLightSensor = lightSensor;
    }
    p.addField("Infrared Light", data.infraRed);
    p.addField("Visible Light", data.visibleLight);

    influxHelper.writeDataPoint(p);
  }

  /*
  // Plant-specific Measurements
  for (auto &plant : plants)
  {
    // plant.measureSensors();
    // Serial.println(plant.lightSensor.measureLight());
  }
  */
}

/* Irrigation Algorithm
  Consider recent Irrigations of PlantGroup and needs of each Plants inside PlantGroup (e.g. Moisture Need, Size of Plant, ...)
  (all Plants that are affected of this Irrigation need to be considered)
  1.1 (Quick) Get PlantGroup Table, check recent Irrigations (InfluxDB) of this Group (e.g. less than 1l today, less than 0.5l in past 4 hours)
  1.2 (Long) Get every single Plant from the PlantGroup and check soilMoisture (do this in State 1 already, with 2nd Core ?)
  3. Check Waterlevel in Tank, if not enough then create failed Irrigation InfluxDB datapoint
  (Reason for Irrigation: ... , Irrigation Amount: ..., Reason for Failure: ...)
  */
void doEvaluate()
{
  wateringNeeded = true;

  // pump1.prepareIrrigation("succulents", 350);
  // pump1.prepareIrrigation("vegetables", 550);
  // pump1.prepareIrrigation("Tomate", 150);
}

void handleHardwareButtons()
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

        if (!wateringNeeded)
        {
          wateringNeeded = true;
        }
        else
        {
          wateringNeeded = false;
        }

        if (fsm.currentState != 4) // otherwise ACTION state before PUMP_IDLE when pressed while Pumping
        {
          fsm.transitionTo(actionState);
        }
      }
    }
  }
  lastButtonState = reading;
}

/*
Read User Commands and Settings, but take Actions later in Action State
(e.g. Water Level needs to be checked first before Pump)
Better Solution: Buttons Requests are directed to Esp32 hosted Server
*/
void checkUserCommands()
{
  char url[50] = "";
  strcat(url, baseUrl);
  strcat(url, "/commands");
  DynamicJsonDocument commands = services.doJSONGetRequest(url);

  // Solenoid
  int relaisChannel = commands[0]["SolenoidValve"][0];
  bool relaisState = commands[0]["SolenoidValve"][1];

  // Irrigation (Plant or Pump)
  const char *irrSubject = commands[0]["Irrigation"][0];
  int irrAmount = commands[0]["Irrigation"][1];

  // Status Light
  const char *display = commands[0]["StatusLight"][0];
  int displayContent = commands[0]["StatusLight"][1];

  // Reset all Commands...
}

void updateIP()
{
  // Send current IP Address, no Tunneling necessary
  char url[50] = "";
  strcat(url, baseUrl);
  strcat(url, "/commands/ip");
  services.doPostRequest(url);
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
  // checkConnections();
  stateBeginMillis = millis();

  // checkWebButtons();

  digitalWrite(Relais[0], HIGH);
  digitalWrite(Relais[1], HIGH);
}

bool countTime(int duration)
{
  return (millis() - stateBeginMillis >= duration * 1000UL);
}

// STATE LOGIC
void on_initState()
{
  Serial.println("Main State Machine runs on Core: ");
  Serial.println(xPortGetCoreID());

  if (fsm.executeOnce)
  {
    commonStateLogic();

    if (!services.getWifiStatus())
    {
      services.setupWifi();
    }

    if (!influxHelper.checkInfluxConnection())
    {
      // influxHelper.setupInflux();
      Serial.println("Couldnt connect to InfluxDB");
    }

    // Run WiFi.begin() before this, or exception
    ButtonHandler::startRestServer();
  }
}

void on_sleepState()
{
  if (fsm.executeOnce)
  {
    didSleep = false;
    commonStateLogic();
    // ESP.deepSleep(30e6); // Connect Sleep Cable AFTER Uploading Code
    // checkConnections();
    // didSleep = true;
  }
  // Simulate Sleep
  // Serial.print(".");
  if (millis() - stateBeginMillis >= SLEEP_INTERVAL * 1000UL)
  {
    didSleep = true;
  }
}

void on_measureState()
{
  if (fsm.executeOnce)
  {
    commonStateLogic();
    // WiFi.disconnect();
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
    updateIP();
    didCycle = true;
  }

  // Run (multiple) sub-StateMachines (Pump, Fan, ...) and wait

  // Check Irrigation Events and run them one after another
  if (wateringNeeded)
  {
    pump1.loop();
  }
  // if(coolingNeeded) {...}
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
  if (countTime(MIN_STATE_DURATION && !wateringNeeded)) // && !fanNeeded && ...
  {
    return true;
  }
  return false;
}

bool transitionS3S4()
{
  if (countTime(MIN_STATE_DURATION && wateringNeeded)) // || fanNeeded || ...
  {
    return true;
  }
  return false;
}

bool transitionS4S1()
{
  // min State Duration must be over AND Pump done, Fan done, ...
  if (wateringNeeded)
  {
    if (!(pump1.lastState == PumpState::DONE))
    {
      return false;
    }
    pump1.currentState = PumpState::IDLE; // Reset Sub StateMachine
  }
  // else if fanNeeded {...}
  else
  {
    if (!countTime(MIN_STATE_DURATION))
    {
      return false;
    }
  }
  return true;
}

void runStateMachine(void *pvParameters)
{
  for(;;) 
  {
    fsm.run();
    delay(1);
  }
}

void setup()
{
  // while (!Serial){}
  Serial.begin(115200);

  // Wire.begin();
  I2Cone.begin(SDA1, SCL1, 200000);
  I2Ctwo.begin(SDA2, SCL2, 100000);

  // Immediately Shut down 2nd ToF, start with different i2C Address later
  pinMode(toF_shut, OUTPUT);
  digitalWrite(toF_shut, LOW);

  // Init (LOW-Trigger) Relais
  for (int i = 0; i < sizeof(Relais); i++)
  {
    pinMode(Relais[i], OUTPUT);
    digitalWrite(Relais[i], HIGH);
  }

  /*
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);

  climate1.setup();
  */

  displayController.setupLEDMatrix();

  Multiplexer::setup();

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
  
  displayController.displayPlantStatus();

  // Check constantly in all States and Sub StateMachines:
  handleHardwareButtons();

  // delay(100);
}