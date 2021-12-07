#include "pump.h"

Pump::Pump(PumpModel &p) : pumpModel(p)
{
    setup();
}

void Pump::setup()
{
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    setupToFSensor();
    maxWaterDistance = 20000;
    currentState = PumpState::IDLE;
}

void Pump::setupToFSensor()
{
  toF.setTimeout(500);
  if (!toF.init())
  {
    Serial.println("Failed to detect and init ToF sensor!");
  }
}

void Pump::loop()
{
    switch (currentState)
    {

    case PumpState::IDLE:

        if (lastState != currentState)
        {
            stateBeginMillis = millis();
            switchOff();
            Serial.println(stateNames[(byte)currentState]);
            waterDistance = toF.readRangeSingleMillimeters();
        }

        if (pumpSignal)
        {
            currentState = PumpState::ON;
            pumpSignal = false;
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
        switchOn();
        Serial.print(".");

        if ((millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL) 
        || pumpSignal == true) // Time is up or Button pressed again (stop now!)
        {
            currentState = PumpState::IDLE;
            pumpSignal = false;
            // Waterlevel was higher before
            distanceDelta = waterDistance - toF.readRangeSingleMillimeters();
            lastPumped = distanceDeltaToMilliliters(distanceDelta);
            if(bestPumped < lastPumped) {bestPumped = lastPumped;}
            totalPumped += lastPumped;
        }

        lastState = PumpState::ON;
        break;
    }
}

bool Pump::checkMinWaterDistance()
{
    return toF.readRangeSingleMillimeters() > maxWaterDistance;
}

float Pump::distanceDeltaToMilliliters(unsigned short distanceDelta)
{
    return distanceDelta * mmToMlFactor;
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

float Pump::getWaterLevel()
{
    if(checkMinWaterDistance())
    {
        Serial.println("Water Level not sufficient. Please refill Tank.");
    }
    return (distanceDeltaToMilliliters(toF.readRangeSingleMillimeters()));
}

void Pump::switchOn()
{
    // Rotation Direction (if H-Bridge)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    analogWrite(enA, 125); // PWM, 0-255
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