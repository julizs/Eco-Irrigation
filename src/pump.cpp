#include "pump.h"

Pump::Pump(PumpModel &p): pumpModel(p){}

void Pump::setup()
{
    currentState = PumpState::IDLE;
}

void Pump::loop()
{
    switch (currentState)
    {
    case PumpState::IDLE:
  
        if(lastState != currentState)
        {
            switchOff();
            Serial.println("Pump is IDLE!");    
        }
        
        if (doPump)
        {
            currentState = PumpState::ON;
            stateBeginMillis = millis(); // Must only be executed once, e.g. right before state transfer
        }

        lastState = PumpState::IDLE;
        break;
    case PumpState::ON:

        // Execute only once
        // Like this or add State turningOn
        if(lastState != currentState)
        {
            Serial.println("Pump is ON!");
        }
        
        // Execute each tick
        switchOn();

        Serial.print(".");
        if (millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL)
        {
            doPump = false;
        }
        if (!doPump) // Time is up or Button Press
        {
            currentState = PumpState::IDLE;
            stateBeginMillis = millis();
        }

        lastState = PumpState::ON;
        break;
    }
}

void Pump::switchOn()
{
    // Rotation Direction (da H-Bridge)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    analogWrite(enA,255);
    //digitalWrite(pumpPin, HIGH); // Relais
}

void Pump::switchOff()
{
    //digitalWrite(pumpPin, LOW); // Relais
}

const Pump::PumpModel& Pump::getPumpModel() const
{
    return pumpModel;
}

void Pump::setPumpModel(const Pump::PumpModel& pM)
{
    pumpModel = pM;
}

void Pump::calibrateFlowRate()
{
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