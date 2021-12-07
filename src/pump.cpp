#include "pump.h"

Pump::Pump(PumpModel &p): pumpModel(p)
{
    setup();
}

void Pump::setup()
{
    pinMode(enA,OUTPUT);
    pinMode(in1,OUTPUT);
    pinMode(in2,OUTPUT);
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
            Serial.println(stateNames[(byte)currentState]);
        }
        
        if (pumpSignal)
        {
            currentState = PumpState::ON;
            pumpSignal = false;
            stateBeginMillis = millis();
        }

        lastState = PumpState::IDLE;
        break;

    case PumpState::ON:

        // Execute once
        if(lastState != currentState)
        {
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
            stateBeginMillis = millis();
        }

        lastState = PumpState::ON;
        break;
    }
}

void Pump::switchOn()
{
    // Rotation Direction (if H-Bridge)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    analogWrite(enA,125); // PWM, 0-255
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
    //
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