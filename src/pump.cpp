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
    
    minStateDuration = 1;
    minWaterDist = 50;
    maxWaterDist = 200;
    currentState = PumpState::IDLE;
}

void Pump::setupToF()
{ 
  // Only if SHT_Pin connected to toF sensor
  pinMode(shut_toF, OUTPUT);
  digitalWrite(shut_toF, LOW);
  digitalWrite(shut_toF, HIGH);

  toF_1.begin(0x52, &I2Cone); // Standard Addr. is 0x29 just like TSL2591, Change via SW)
  //toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // Often results in Measurement Error/Timeout
  toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
  Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds()); // 200k micro sec (0.2 sec) on High Accuracy profile
  //toF_1. setMeasurementTimingBudgetMicroSeconds(300000); // increase to 300k
  Serial.println("did Setup VL530x");
}


bool Pump::checkFillLevel()
{
    int fillLevel = readToF();
    return fillLevel < maxWaterDist && fillLevel < minWaterDist;
}

int Pump:: readToF()
{
  // Do 2 valid Reading and only then calc Average
  // 3 Attemps per single valid Reading (or e.g. time limit while in IDLE State), then stop
  // measure.RangeStatus == 4 means Out of Range
  int avgDistance = 0;
  int sampleNum = 2;
  VL53L0X_RangingMeasurementData_t measure;

  for(int i = 0; i < sampleNum; i++)
  { 
    int distance = 0;
    bool validReading = false;
    int j = 0;

    while(j < 3 && !validReading) {
        if((measure.RangeStatus == 4 || distance < minWaterDist || distance > maxWaterDist))
        {
            toF_1.rangingTest(&measure, false);
            distance = measure.RangeMilliMeter;
            Serial.print("Reading (mm): ");
            Serial.print(distance);
            j++; 
        }
        else
        {
            Serial.println(" is valid");
            validReading = true;
            avgDistance += distance;
        }
        delay(500); 
    }
  }
  
  avgDistance/= (sampleNum * 1.0f);
    
  return avgDistance;
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
            switchOff();
            validFillLevel = checkFillLevel();
            cycleDone = true; // IDLE -> 1s -> ON -> IDLE
        }

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && pumpSignal)
        {
            if(true) // validFillLevel, Core1 Panicked
            {
                currentState = PumpState::ON;
                //pumpSignal = false;
            }
            else
            {
                Serial.println("Not enough Water for Irrigation.");
                currentState = PumpState::DONE;
            } 
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
        // 2 Checks for Safety: Water Distance and Max. Pump Time
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
            // if(bestPumped < lastPumped) {bestPumped = lastPumped;}
            // totalPumped += lastPumped;
        }

        lastState = PumpState::ON;
        break;

        case PumpState::DONE:

        if (lastState != currentState)
        {
            pumpSignal = false;
            stateBeginMillis = millis();
            Serial.println(stateNames[(byte)currentState]);

            // Waterlevel was higher before
            // int oldWaterDistance = currWaterDist;
            // currWaterDist = readToF();
            // Update Pumped Values in InfluxDB
        }

        lastState = PumpState::DONE;
        break;
    }
}

/*
bool Pump::checkPumpPerformance(unsigned short lastPumped)
{
    // Correct: bestPumped struct per Model and Voltage or PWM value (e.g. Palermo, 100ml, 125)
    if(bestPumped - lastPumped > 50) // 50 Milliliter Threashold
    {
        Serial.println("Pump Performance deteriorated, need to clean Pump Filter or Exchange Water.");
        return true;
    }
    return false;
}
*/

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