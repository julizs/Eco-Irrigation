#include "pump.h"

Pump::Pump(PumpModel &p) : pumpModel(p)
{
    setup();
}

void Pump::setup()
{
    //pinMode(enA, OUTPUT);
    //pinMode(in1, OUTPUT);
    //pinMode(in2, OUTPUT);

    //setupToF();
    
    minStateDuration = 1;
    maxWaterDistance = 20000;
    currentState = PumpState::IDLE;
}

void Pump::setupToF()
{ 
  // Only if SHT_Pin connected to toF sensor
  pinMode(shut_toF, OUTPUT);
  digitalWrite(shut_toF, LOW);
  digitalWrite(shut_toF, HIGH);

  toF_1.begin(0x52, &I2Cone); // wie bei TSL2591 Standard Addr. 0x29, Änderung per Software nötig)
  //toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // Often results in Measurement Error/Timeout
  toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
  Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds()); // 200k micro sec (0.2 sec) on High Accuracy profile
  //toF_1. setMeasurementTimingBudgetMicroSeconds(300000); // increase to 300k
  Serial.println("did Setup VL530x");
}

int Pump:: readToF()
{
  delay(500);
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Reading a measurement... ");
  toF_1.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
  } else {
    Serial.println(" out of range ");
  }
    
  return measure.RangeMilliMeter;
}

void Pump::loop()
{
    switch (currentState)
    {

    case PumpState::IDLE:

        if (lastState != currentState)
        {
            stateBeginMillis = millis();
            Serial.println(stateNames[(byte)currentState]);
            //waterDistance = toF_1.readRangeSingleMillimeters();
            switchOff();
            cycleDone = true; // IDLE -> 1s -> ON -> IDLE
        }

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && pumpSignal)
        {
            currentState = PumpState::ON;
            //pumpSignal = false;
        }

        lastState = PumpState::IDLE;
        break;

    case PumpState::ON:

        // Execute once
        if (lastState != currentState)
        {
            cycleDone = false;
            pumpSignal = false;
            stateBeginMillis = millis();
            Serial.println(stateNames[(byte)currentState]);
        }

        // Execute each tick
        switchOn();
        Serial.print(".");

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL &&
        ((millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL)
        || pumpSignal == true)) // minStateTime is up AND (Time is up OR Button pressed again (Stop now!))
        {
            currentState = PumpState::IDLE;
            pumpSignal = false;
            // Waterlevel was higher before
            // distanceDelta = waterDistance - toF_1.readRangeSingleMillimeters();
            // lastPumped = distanceDeltaToMilliliters(distanceDelta);
            if(bestPumped < lastPumped) {bestPumped = lastPumped;}
            totalPumped += lastPumped;
        }

        lastState = PumpState::ON;
        break;
    }
}

bool Pump::checkPumpPerformance(unsigned short lastPumped)
{
    // Correct: bestPumped struct per Model and PWM value (e.g. Palermo, 100ml, 125)
    if(bestPumped - lastPumped > 50) // 50 Milliliter Threashold
    {
        Serial.println("Pump Performance deteriorated, need to clean Pump Filter or Exchange Water.");
        return true;
    }
    return false;
}

/*
bool Pump::sufficientWaterLevel()
{
    //return toF_1.readRangeSingleMillimeters() > maxWaterDistance;
}

float Pump::currentwaterLevel()
{
    if(checkMinWaterDistance())
    {
        Serial.println("Water Level not sufficient. Please refill Tank.");
    }
    //return (distanceDeltaToMilliliters(toF.readRangeSingleMillimeters()));
}
*/ 

void Pump::switchOn()
{
    /*
    // Set Rotation Direction (if H-Bridge and Pump supports 2 Dirs)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    */

    //analogWrite(enA, 125); // PWM, 0-255, ESP32 Befehl?

    //digitalWrite(pumpPin, HIGH); // Relais
}

void Pump::switchOff()
{
    //digitalWrite(pumpPin, LOW); // Relais
}

const Pump::PumpModel &Pump::getPumpModel() const
{
    return pumpModel;
}

void Pump::setPumpModel(const Pump::PumpModel &pM)
{
    pumpModel = pM;
}




// Nested Class Definitions

Pump::PumpModel::PumpModel(byte minVoltage, byte maxVoltage, byte maxPumpingDuration, int flowRate)
{
    this->minVoltage = minVoltage;
    this->maxVoltage = maxVoltage;
    this->maxPumpingDuration = maxPumpingDuration;
    this->flowRate = flowRate;
}

byte Pump::PumpModel::getminVoltage()
{
    return this->minVoltage;
}

// Tankfüllstand
// Pumpen Funktionstest
// Durchlauftest (läuft Pumpe mit initialier/voller FlowRate?)
// Kalibrierung (Wieviel Wasser je Sekunde?)
// Wassertausch, Pumpenreinigung nach 1 Monat
// PWM / Voltage Einstellungen, autom. Prüfung je nach Modell