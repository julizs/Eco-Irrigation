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
    measureIntervall = 1000; // Ms

    setupPWM();
}

void Pump::setupPWM()
{
    // Setup PWM Channel
    pinMode(pwmPin, OUTPUT);
    ledcSetup(pwmChannel, frequency, resolution);
    ledcAttachPin(pwmPin, pwmChannel);
}

void Pump::loop()
{
    switch (currentState)
    {

    case PumpState::IDLE:

        if (lastState != currentState)
        {
            commonStateLogic();
  
            if(!cistern.toF_ready)
            {
                // Redo Setup...
                // cistern.setupToF();
                setupToFs();
            }     
        }

        if (countTime(minStateDuration))
        {
            if (!cistern.toF_ready)
            {
                errorCode = 1;
                currentState = PumpState::ABORT;
            }
            /*
            Pump SM will not even get started if no valid Solenoid/Pump not active in System
            else if(relaisChannel == -1)
            {
                errorCode = 5;
                currentState = PumpState::ABORT;
            }
            */
            /*
            else if(!Irrigation::validSolenoid(relaisChannel, Irrigation::waterLimit24h, 24))
            {
                // Check waterLimits per SolenoidValve (on Button Press by User)
                // Data needs to be provided first (Utilities::provideData())
                errorCode = 2;
                currentState = PumpState::ABORT;
            }*/
            else
            {
                cistern.updateWaterLevel();
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


        // Repeating Measurements
        currentTime = millis();
        if(currentTime >= (lastTime + measureIntervall))
        {
            lastTime = currentTime;

            cistern.meter.measureFlow();

            powerMeter1.measureIna();
        }
        
        /*
        // Button pressed
        if (pumpButton == true)
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

            // Measure Power once before stopping Pump
            // writeIna();

            switchOff();

            cistern.meter.measureVolume();

            cistern.driveSolenoid(relaisChannel, HIGH);

            // If ToF not correctly Setup (but Status == ERROR_NONE) -> Crash
            cistern.waterManagement();
        }

        lastState = PumpState::DONE;
        break;

    case PumpState::ABORT:
        if (lastState != currentState)
        {
            commonStateLogic();

            // e.g. if Pump Process aborted midtime by Button -> DONE, not ABORT 
            // cistern.waterManagement(relaisChannel);

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