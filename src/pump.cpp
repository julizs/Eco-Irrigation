#include "Pump.h"

Pump::Pump(int pwmChannel, int pwmPin, Cistern& c) : pumpModel(2048), cistern(c)
{
    this->pwmChannel = pwmChannel;
    this->pwmPin = pwmPin;
    frequency = 30000;
    resolution = 8; // Bits
    dutyCycle = 200;
    setup();
}

void Pump::setup()
{
    /*
    H-Bridge Direction
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    */

    minStateDuration = 4;
    maxStateDuration = 10;
    currentState = PumpState::IDLE;

    setupPWM();
}

void Pump::setupPWM()
{
    // Setup PWM Channel
    pinMode(pwmPin, OUTPUT);
    ledcSetup(pwmChannel, frequency, resolution);
    ledcAttachPin(pwmPin, pwmChannel);
}

bool Pump::setupIna()
{
    if (!ina219.begin(&I2Ctwo))
    {
        Serial.println(F("Failed to boot Ina219_1"));
        return false;
    }
    return true;
}

INAdata Pump::readIna()
{
    INAdata data;
    data.busVoltage = ina219.getBusVoltage_V();
    data.shuntVoltage = abs(ina219.getShuntVoltage_mV() / 1000.0f);
    data.voltage = data.busVoltage + data.shuntVoltage;
    // Constrain to 0 - 10 Ampere, otherwise infinity bug when on USB, InfluxDB fail to write inf
    data.current = abs(constrain(ina219.getCurrent_mA(), 0, 10000) / 1000.0f);
    // power_mW = ina219.getPower_mW();
    data.power = data.busVoltage * data.current; // P = U * I

    return data;
}

void Pump::writeIna()
{
    // Tags?
    // p.clearFields();
    INAdata inaData = readIna();
    p0.addField("voltage", inaData.voltage);
    p0.addField("current", inaData.current);
    p0.addField("power", inaData.power);
    p0.addField("busVoltage", inaData.busVoltage);
    p0.addField("shuntVoltage", inaData.shuntVoltage);
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

            if(cistern.toF.Status != 0)
            {
                cistern.setupToF();
            }
        }

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && wateringNeeded)
        {
            if (cistern.toF.Status == 0)
            {
                if (cistern.validWaterLevel())
                {
                    currentState = PumpState::ON;
                }
                else
                {
                    Serial.println("Not enough Water, Irrigation falied.");
                    currentState = PumpState::DONE;
                }
                // In both Cases
                // cistern.updateWaterLevel();
            }
            else
            {
                Serial.println("ToF Sensor can't be Setup, Irrigation failed.");
                currentState = PumpState::DONE;
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

            // Drive Relais
        }

        switchOn();

        // minStateTime is up AND (Time is up OR Button pressed again (Stop now!))
        // Pump shall be in ON State for max. 10s (maxStateTime)
        if (millis() - stateBeginMillis >= minStateDuration * 1000UL &&
            ((millis() - stateBeginMillis >= 10 * 1000UL) || wateringNeeded == false)) 
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

            if (cistern.toF.Status != 0)
            {
                // Or Crash if ToF not correctly Setup
                return;
            }

            // Measure Power before stopping Pump
            writeIna();

            delay(100);

            wateringNeeded = false;

            switchOff();

            // Do InfluxDB Updates AFTER turning off pump
            cistern.updateWaterLevel();

            // Finally, Write only once
            influxHelper.writeDataPoint(p0);
            
            cistern.updateIrrigations(); // Datapoint p2
        }

        lastState = PumpState::DONE;
        break;
    }
}

void Pump::switchOn()
{
    /*
    // H-Bridge, set Pump Direction
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    */

    // Esp8266
    // analogWrite(enA, 125);

    // Esp32
    // Run Pump1 with 5V (12V Input L298N, 10V Output at max. PWM Duty)
    ledcWrite(pwmChannel, 255);
}

void Pump::switchOff()
{
    ledcWrite(pwmChannel, 0);
}

/*
Pump::checkPumpPerformance(unsigned short lastPumped)
{
    // Correct: bestPumped struct per Model and Voltage or PWM value (e.g. Palermo, 100ml, 50% PWM)
    if(lastPumped > bestPumped)
    {
        bestPumped = lastPumped;
    }
    if(bestPumped - lastPumped > 50) // 50 Milliliter Threashold
    {
        Serial.println("Pump Performance deteriorated, need to clean Pump Filter or Exchange Water.");
    }
}
*/