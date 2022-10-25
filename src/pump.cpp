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
    transCount = 0;
    maxSelfTrans = 3;

    // PWM Details
    frequency = 30000;
    resolution = 8; // bits
    dutyCycle = 200;
    setupPWM();

    lastState = (PumpState)-1;
    pumpTime = constrain(pumpTime, 2.0f, 15.0f);
    measureIntervall = 1000; // ms
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

    case PumpState::INIT:

        if (lastState != currentState)
        {
            commonStateLogic();

            // Apply Usersettings from REST (e.g. dutyCycle)
            // setup();

            if(!cistern.toF_ready())
                cistern.setupToF();
        }

        if (countTime(minStateDuration))
        {
            // All setup attemps failed
            if (!cistern.toF_ready())
            {
                if(transCount < maxSelfTrans)
                {
                    transCount++;
                    cistern.setupToF();
                    currentState = PumpState::INIT;
                }
                else
                {
                    errorCode = 1;
                    currentState = PumpState::ABORT;
                }  
            }
            // Should this check also be done before, in Irrigation class?
            else if(!cistern.validWaterLevel(allocatedWater))
            {
                errorCode = 3;
                currentState = PumpState::ABORT;
            }
            else
            {
                errorCode = 0; // None
                currentState = PumpState::ON;
            }
        }

        lastState = PumpState::INIT;
        break;

    case PumpState::ON:

        // Call only once, entry / State Function
        if (lastState != currentState)
        {
            commonStateLogic();
            // cistern.readToF_cont();

            // Details Relais-Shield (NO)
            // https://randomnerdtutorials.com/esp8266-relay-module-ac-web-server/
            cistern.driveSolenoid(relaisChannel, LOW);
        }

        // do / State Function
        switchOn();

        // Repeat Measurements in Intervall
        currentTime = millis();
        if (currentTime >= (lastTime + measureIntervall))
        {
            lastTime = currentTime;

            cistern.meter.measureFlow();
            cistern.meter.writePoint();

            powerMeter1.measureIna();
            powerMeter1.writePoint();
        }

        /*
        // User presses HW-Button to cancel, measure Water
        if (pumpButton == true)
        {
            currentState = PumpState::DONE;
        }
        */

        // Check constantly
        if (countTime(minStateDuration) && countTime(pumpTime))
        {
            // exit / State Function
            currentState = PumpState::DONE;
        }

        lastState = PumpState::ON;
        break;

    case PumpState::DONE:

        if (lastState != currentState)
        {
            commonStateLogic();

            switchOff();

            cistern.driveSolenoid(relaisChannel, HIGH);

            cistern.meter.measureVolume();      

            // Only measure if Water was pumped
            // (toF must be setup correctly, or crash here)
            // if(lastState == PumpState::ON)
            cistern.waterManagement();
        }

        lastState = PumpState::DONE;
        break;

    case PumpState::ABORT:
        if (lastState != currentState)
        {
            commonStateLogic();

            printError();
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
    /*
    Details ledc for PWM:
    https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html
    https://diyi0t.com/arduino-pwm-tutorial/
    Details L298N, Direction + PWM:
    https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/
    Max 255 = 3.3V Output Voltage (Esp32) statt 5V (Jumper), daher Pumpe zu schwach? (L298N: 10V Output @12V Input @ 5V ENA/B Pin)
    */
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
    transCount++;
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

/*
Reset Machine for next run
lastState must be != currentState for entry Logic to run once
*/
void Pump::resetMachine()
{
    currentState = PumpState::INIT;
    lastState = (PumpState)-1;
}

void Pump::printError()
{
    Serial.println(errors[errorCode]);
}


/*
All Funcs of State DONE could also be implemented
in exit Func. of ON State
*/

/*
lastState = (PumpState)-1;
Enums cannot be set to null, initial Value is always 0 (so PumpState.IDLE)
Set to -1 so that lastState != currentState gets triggered after Bootup

Problem: cistern.toF.Status != VL53L0X_ERROR_NONE is true, even if no Setup ->
Crash when Measuring if not also toF.ready checked
*/

/*
User cancels Pump Process with HW-Button (?)
-> DONE State, not ABORT (measure Water)
*/

/*
Gets checked already in Irrigation class
no valid Solenoid / Pump is not active in System
else if(solenoidValve == -1)
{
    errorCode = 5;
    currentState = PumpState::ABORT;
}
else if(!Irrigation::validSolenoid(relaisChannel, Irrigation::waterLimit24h, 24))
{
    // Check waterLimits per SolenoidValve (on Button Press by User)
    // Data needs to be provided first (Utilities::provideData())
    errorCode = 2;
    currentState = PumpState::ABORT;
}*/