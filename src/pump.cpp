#include "Pump.h"

Pump::Pump(int pwmChannel, int pwmPin, Cistern &c) : pumpModel(1024), cistern(c)
{
    this->pwmChannel = pwmChannel;
    this->pwmPin = pwmPin;
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
    frequency = 30000;
    resolution = 8; // Bits
    dutyCycle = 200;
    lastState = (PumpState)-1;
    pumpTime = constrain(pumpTime, 2.0f, 15.0f);

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
    // Constrain (InfluxDB Write fail, Current = Inf)
    float current = abs(ina219.getCurrent_mA() / 1000.0f);
    data.current = constrain(current, 0, 2);
    // power_mW = ina219.getPower_mW();
    data.power = data.busVoltage * data.current; // P = U * I

    return data;
}

void Pump::writeIna()
{
    // p.clearFields();
    INAdata inaData = readIna();
    p0.addField("voltage", inaData.voltage);
    p0.addField("current", inaData.current);
    p0.addField("power", inaData.power);
    p0.addField("busVoltage", inaData.busVoltage);
    p0.addField("shuntVoltage", inaData.shuntVoltage);
    delay(200);
}

void Pump::loop()
{
    switch (currentState)
    {

    case PumpState::IDLE:

        if (lastState != currentState)
        {
            commonStateLogic();

            // Crashes when &pump2 put on action Stack
            // setupToFs();

            // Check recent Irrigations for Limits (incase of User Button Press)
            /*
            if(!Irrigation::validSolenoid())
            {
                currentState = PumpState::ABORT;
            }
            */
        }

        if (countTime(minStateDuration))
        {
            if (cistern.toF.Status != VL53L0X_ERROR_NONE)
            {
                errorCode = 1;
                currentState = PumpState::ABORT;
            }
            /*
            else if(recentIrrigations.isNull())
            {
                errorCode = 2;
                currentState = PumpState::ABORT;
            }
            */
            else
            {
                if (cistern.validWaterLevel())
                {
                    errorCode = 0; // None
                    currentState = PumpState::ON;
                }
                else
                {
                    errorCode = 3;
                    currentState = PumpState::ABORT;
                }
            }
        }

        lastState = PumpState::IDLE;
        break;

    case PumpState::ON:

        if (lastState != currentState)
        {
            commonStateLogic();
            // Call only once
            // cistern.readToF_cont();
        }

        cistern.driveSolenoid(relaisChannel, LOW);
        switchOn();
        // cistern.measureWaterFlow();

        /*
        // Button pressed
        if (wateringNeeded == false)
        {
            errorCode = 4;
            currentState = PumpState::ABORT;
        }
        */

        // Check constantly
        if (countTime(minStateDuration) && countTime(pumpTime))
        {
            currentState = PumpState::DONE;
        }

        lastState = PumpState::ON;
        break;

    case PumpState::DONE:

        if (lastState != currentState)
        {
            commonStateLogic();

            // Measure Power before stopping Pump
            writeIna();

            switchOff();
            cistern.driveSolenoid(relaisChannel, HIGH);

            #if (SENDDATA == 1)
            {
            // Write Point (Ina and ToF Data) to Buffer
            influxHelper.writeDataPoint(p0);

            // If ToF not correctly Setup (but Status == ERROR_NONE) -> Crash
            cistern.updateIrrigations(relaisChannel);
            }
            #endif
        }

        lastState = PumpState::DONE;
        break;

    case PumpState::ABORT:
        if (lastState != currentState)
        {
            commonStateLogic();
            Serial.println(errors[errorCode]);
        }

        lastState = PumpState::ABORT;
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

bool Pump::countTime(int durationSec)
{
    return (millis() - stateBeginMillis >= durationSec * 1000UL);
}

void Pump::commonStateLogic()
{
    stateBeginMillis = millis();
    Serial.println(stateNames[(byte)currentState]);
}

void Pump::add_callback(callback func)
{
    setupToFs = func;
}

bool Pump::isDone()
{
    // lastState instead of currentState, so that Logic of last State also gets exec
    return lastState == PumpState::DONE || lastState == PumpState::ABORT;
}

void Pump::resetMachine()
{
    currentState = PumpState::IDLE;
    // lastState must be != currentState for ExecOnce Logic to run
    lastState = (PumpState)-1;
}

/*
NO other Instructions inside loop(), gets called every Instruction

lastState = (PumpState)-1;
Enums cannot be set to null, initial Value is always 0 (so PumpState.IDLE)
Set to -1 so that lastState != currentState gets triggered after Bootup

Problem: cistern.toF.Status != VL53L0X_ERROR_NONE is true, even if no Setup ->
Crash when Measuring if not also toF.ready checked
*/