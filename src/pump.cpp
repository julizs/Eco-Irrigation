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
    maxWaterDist = 300;
    currentState = PumpState::IDLE;

    setupPWM();
}

void Pump::setupPWM()
{
    // Setup 2 PWM Channels for Pumps
    // https://randomnerdtutorials.com/esp32-pwm-arduino-ide/
    // https://diyi0t.com/arduino-pwm-tutorial/
    ledcSetup(1, 5000, 8); // Channel, Frequency, Resolution (8 Bit, 0-255)
    ledcSetup(2, 5000, 8);
    ledcAttachPin(pumpPWM_Pin_1, 0);
    ledcAttachPin(pumpPWM_Pin_2, 1);
}

bool Pump::setupToF()
{ 
  // Only if SHT_Pin connected to toF sensor
  //pinMode(shut_toF, OUTPUT);
  //digitalWrite(shut_toF, LOW);
  //digitalWrite(shut_toF, HIGH);

  // Standard Addr. is 0x29 (just like TSL2591) on each Boot, Change via SW)
  if(!toF_1.begin(0x52, &I2Cone))
  {
    Serial.println(F("Failed to boot VL53L0X"));
    return false;
  }

  toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
  //toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // Often results in Measurement Error/Timeout
  //Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds()); // 200k micro sec (0.2 sec) on High Accuracy profile
  //toF_1. setMeasurementTimingBudgetMicroSeconds(300000); // increase to 300k

  return true;
}


bool Pump::checkWaterLevel()
{
    int waterLevel = readToF();
    if(waterLevel < maxWaterDist && waterLevel > minWaterDist)
    {
        // Update only valid Values
        currWaterDist = waterLevel;
        return true;
    }
    return false;
}

int Pump:: readToF()
{
  // Do 2 valid Reading and only then calc Average
  // 3 Attemps per single valid Reading (or e.g. time limit while in IDLE State), then stop
  // measure.RangeStatus == 4 means Out of Range
  int avgDistance = 0;
  int sampleNum = 4;
  int readings [sampleNum] = { };
  VL53L0X_RangingMeasurementData_t measure;

  for(int i = 0; i < sampleNum; i++)
  { 
    int distance = 0;
    bool validReading = false;
    int j = 0; // 3 Attemps per single Reading

    while(j < 3 && !validReading) {
        if((measure.RangeStatus == 4 || distance < minWaterDist || distance > maxWaterDist))
        {
            //toF_1.rangingTest(&measure, false);
            //distance = measure.RangeMilliMeter;
            distance = toF_1.readRange();
            Serial.print("Reading (mm): ");
            Serial.print(distance);
            j++; 
        }
        else
        {
            Serial.println(" is valid");
            readings[i] = distance;
            validReading = true;
            avgDistance += distance;
        }
        delay(500); 
    }
  }
  
  avgDistance/= (sampleNum * 1.0f);
    
  return avgDistance;
}

int Pump::readToF_cont()
{
    toF_1.startRangeContinuous(100);
    Serial.println(toF_1.readRange());
    toF_1.stopRangeContinuous();
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
            ToF_ready = setupToF();
        }

        //readToF_cont();

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && wateringNeeded)
        {
            if(ToF_ready)
            {
                if(checkWaterLevel()) // Update both currWaterDist and check if valid with same 1 Reading
                {
                    currentState = PumpState::ON;
                    ToF_ready = false; // For next iteration
                }
                else
                {
                    Serial.println("Not enough Water, skipping Irrigation.");
                    currentState = PumpState::DONE;
                } 
            }
            else
            {
                // Try to setup again
                ToF_ready = setupToF();
                delay(100);
            }   
        }

        lastState = PumpState::IDLE;
        break;

    case PumpState::ON:

        // Execute once
        if (lastState != currentState)
        {
            stateBeginMillis = millis();
            Serial.println(stateNames[(byte)currentState]);
        }

        // Execute each tick
        // 2 Checks for Safety: Water Distance and Max. Pump Time
        switchOn();

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL &&
        ((millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL)
        || wateringNeeded == false)) // minStateTime is up AND (Time is up OR Button pressed again (Stop now!))
        {
            currentState = PumpState::DONE;
        }

        lastState = PumpState::ON;
        break;

    case PumpState::DONE:

        if (lastState != currentState)
        {
            stateBeginMillis = millis();
            Serial.println(stateNames[(byte)currentState]);

            wateringNeeded = false;
            switchOff();

            int oldWaterDistance = currWaterDist;
            currWaterDist = readToF();
            int pumpedWater = oldWaterDistance - currWaterDist;
            // Send Values in InfluxDB for Persistence
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
    // Set Rotation Direction (only if H-Bridge and Pump supports 2 Directions)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    */

    // Esp8266
    //analogWrite(enA, 125); 

    // Esp32
    ledcWrite(1, 255); // Run Pump1 with 5V (12V Input L298N, 10V Output at max. PWM Duty)
    ledcWrite(2, 255);
    
    //digitalWrite(pumpPin, HIGH); // via Relais
}

void Pump::switchOff()
{
    //digitalWrite(pumpPin, LOW); // via Relais
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