#include <main.h>
#include <AmbientClimate.h>
#include <SoilMoisture.h>
#include <AmbientLight.h>
#include <Pump.h>
#include <StatusDisplay.h>
#include <LinkedList.h>
#include <StateMachine.h>
#include <ArduinoJson.h>

Services services;
InfluxHelper influxHelper;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

//SoilMoisture soilMoisture1(1100, 0);
//SoilMoisture soilMoisture2(1100, 1);
//Fotoresistor fotoResistor1(10000, 3.3, 10, C15);
AmbientClimate climate1(500, 2);
AmbientLight lightSensor1(1);
AmbientLight lightSensor2(2);
Pump::PumpModel qr50e(12, 12, 4, 240);
Pump::PumpModel palermo(6, 12, 6, 330);
Pump pump1(qr50e);
StatusDisplay displayController;

//Plant plant1(lightSensor2, soilMoisture1);
//Plant plant2(lightSensor2, soilMoisture2);
//std::vector<Plant> plants{plant1, plant2};

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

void doMeasurements()
{
  scanI2CBus(&I2Cone);
  scanI2CBus(&I2Ctwo);

  // ESP32-SPECIFIC (GLOBAL) MEASUREMENTS
  // Reuse datapoint, add tags and clear fields
  if (!p0.hasTags())
  {
    p0.addTag("device", DEVICE);
    p0.addTag("SSID", WiFi.SSID());
  }

  p0.clearFields();

  pump1.setupToF_1();
  pump1.setupToF_2();
  int waterLevel = pump1.readToF(pump1.toF_1);
  int waterLevel2 = pump1.readToF(pump1.toF_2);
  //pump1.readToF_cont();
  p0.addField("water level bucket 1", waterLevel);
  p0.addField("water level bucket 2", waterLevel2);

  byte rssi = WiFi.RSSI();
  p0.addField("rssi", rssi);

  // Idle Power Consumption of L298N
  INAdata inaData = pump1.readIna_1();
  p0.addField("voltage", inaData.voltage);
  p0.addField("current", inaData.current);
  p0.addField("power", inaData.power);
  p0.addField("busVoltage", inaData.busVoltage);
  p0.addField("shuntVoltage", inaData.shuntVoltage);

  pump1.setupIna_1();
  //INAdata data = pump1.readIna_1();

  influxHelper.writeDataPoint(p0);

  // PLANT-SPECIFIC MEASUREMENTS
  // 1. Get Plant-Sensor Assignments/Configs
  char url[] = "https://juli.uber.space/node/plants";
  DynamicJsonDocument doc = services.doJSONGetRequest(url);

  // 2. Setup Sensors only once
  lightSensor1.setupTSL2591(I2Cone);
  lightSensor2.setupTSL2591(I2Ctwo);

  // 3. Measure the assigned Sensors from each plant
  // (First check assignments in each measure iteration since user could have changed them
  // and only measure the assigned ones)
  Point p("Plant Data"); // Write new data point/row into this table, reuse datapoint
  int lastLightSensorNum;
  TSL2591data data;

  // For each plant
  for (int i = 0; i < doc.size(); i++)
  {
    String plantName = doc[i]["plantName"].as<String>();
    p.clearTags();
    p.clearFields();
    p.addTag("Plant", plantName);

    Serial.println();
    Serial.println(doc[i]["_id"].as<String>());
    Serial.println(doc[i]["plantName"].as<String>());

    // Remove any r["_field"] Filter in InfluxDB Cell Script Editor so all added
    // or removed Sensorvalues are shown
    for (int j = 0; j < sizeof(doc[i]["moistureSensors"]); j++)
    {
      int moistureSensor = doc[i]["moistureSensors"][j].as<int>();
      if (moistureSensor != 0) // ignore 0s, e.g. 67000 (=channels 6,7)
      {
        // Serial.print("Measure Moisture: ");
        // Serial.print(moistureSensor);
        char key[25];
        sprintf(key, "Soil Moisture Sensor %d", moistureSensor);
        // Measure moistureSensor-1 (User enters 1-16, Multiplexer reads 0-15)
        int value = SoilMoisture::measureSoilMoistureSmoothed(moistureSensor - 1);
        p.addField(key, value);
      }
    }

    // TODO Multiple lightSensors, select the assigned one
    // Only remeasure lightSensor of plant if sensor wasnt already measured
    int currentLightSensorNum = doc[i]["lightSensor"].as<int>();
    //if(i != 0 && lastLightSensorNum != currentLightSensorNum)
    //{
    data = lightSensor2.measureLight();
    //}
    p.addField("Infrared Light", data.infraRed);
    p.addField("Visible Light", data.visibleLight);

    influxHelper.writeDataPoint(p);

    Serial.println();
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

void doEvaluate()
{
  wateringNeeded = true;
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

  digitalWrite(RELAIS_1, HIGH);
  digitalWrite(RELAIS_2, HIGH);
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
      //influxHelper.setupInflux();
      Serial.println("Couldnt connect to InfluxDB");
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
  }

  //digitalWrite(RELAIS_1, LOW);
  digitalWrite(RELAIS_2, LOW);

  // run sub-StateMachines
  if (wateringNeeded)
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
  // min State Duration must be over AND Pump done, Fan done, ...
  if (countTime(MIN_STATE_DURATION) && pump1.lastState == PumpState::DONE)
  {
    pump1.currentState = PumpState::IDLE; // Reset Sub StateMachine
    return true;
  }
  return false;
}

void setup()
{
  // while (!Serial){}
  Serial.begin(115200);

  //Wire.begin();
  I2Cone.begin(SDA1, SCL1, 200000);
  I2Ctwo.begin(SDA2, SCL2, 100000);
  delay(100);

  // Immediately Shut down 2nd ToF to start with different i2C Address later
  pinMode(shut_toF, OUTPUT);
  digitalWrite(shut_toF, LOW);

  // LOW-Trigger Relais
  pinMode(RELAIS_1, OUTPUT);
  pinMode(RELAIS_2, OUTPUT);
  digitalWrite(RELAIS_1, HIGH);
  digitalWrite(RELAIS_2, HIGH);

  /*
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);

  climate1.setup();
  */

  displayController.setupLEDMatrix();

  Multiplexer::setup();

  //plants[0].setName("Thymian");
  //plants[1].setName("Orchidee");

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
  
  displayController.displayPlantStatus();

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