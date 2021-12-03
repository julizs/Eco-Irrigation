#include "pump.h"

Pump::Pump(PumpModel &p): pumpModel(p){}


void Pump::Update()
{
    switch (state)
    {
    case PumpState::IDLE:
        stateBeginMillis = millis();
        switchOff();
        if (doPump)
        {
            state = PumpState::ON;
        }
        break;
    case PumpState::ON:
        stateBeginMillis = millis();
        switchOn();
        if (millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL)
        {
            doPump = false;
        }
        if (!doPump) // Time is up or Button Press
        {
            state = PumpState::IDLE;
        }
    }
}

void Pump::switchOn()
{
    digitalWrite(pumpPin, HIGH);
}

void Pump::switchOff()
{
    digitalWrite(pumpPin, LOW);
}

/* Pump::PumpModel* Pump::GetPumpModel()
{
    return pumpModel;
} */

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
// Pumpentest
// Durchlauftest (läuft Pumpe mit vollem Durchlauf?)
// Kalibrierung Durchlauf (Wieviel Wasser je Sekunde?)
// Wassertausch, Pumpenreinigung nach 1 Monat
// PWM / Voltage Einstellungen, autom. Prüfung je nach Modell