#include "pump.h"

Pump::Pump(PumpModel pumpModel)
{
    this->pumpModel = pumpModel;
}

struct Pump::PumpModel
{
    char* name = "Palermo";
    byte minVoltage = 6;
    byte maxVoltage = 12;
    byte maxPumpingDuration = 30;
    int measuredFlowRate = 330;
    byte getMinVoltage() {return minVoltage;}
};

void Pump::Update()
{
    switch (state)
    {
    case PumpState::IDLE:
        stateBeginMillis = millis();
        if (doPump)
        {
            state = PumpState::ON;
        }
        break;
    case PumpState::ON:
        stateBeginMillis = millis();
        if(millis() - stateBeginMillis >= pumpModel.maxPumpingDuration)
        {
            doPump = false;
        }
        if(!doPump) // Time is up or Button Press
        {
            state = PumpState::IDLE;
        }
    }
}

void Pump::switchOn()
{

}

void Pump::switchOff()
{

}

void Pump::calibrateFlowRate()
{

}

// Tankfüllstand
// Pumpentest
// Durchlauftest (läuft Pumpe mit vollem Durchlauf?)
// Kalibrierung Durchlauf (Wieviel Wasser je Sekunde?)
// Wassertausch, Pumpenreinigung nach 1 Monat
// PWM / Voltage Einstellungen, autom. Prüfung je nach Modell