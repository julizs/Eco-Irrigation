#include "Pump.h"

// FlowMeter flow(flowPin);

Pump::Pump(Cistern &c) : cistern(c)
{
    flow = new FlowMeter(flowPin1);
    setup();
    setupPWM();
}

void Pump::setup()
{
    minStateDuration = 4;
    transCount = 0;
    maxSelfTrans = 3;
    measureIntervall = 1000; // ms
    lastState = (PumpState)-1;
    
    /*
    H-Bridge Direction
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    */
}

// Setup ALL pwmPins for Pumps
void Pump::setupPWM()
{
    frequency = 30000;
    resolution = 8; // bits
    dutyCycle = 200;

    for(int i = 0; i < 2; i++) // num of pumps
    {
        pinMode(pwmPins[i], OUTPUT);
        ledcSetup(i, frequency, resolution);
        ledcAttachPin(pwmPins[i], i);
    }   
}

void Pump::loop()
{
    switch (currentState)
    {

    case PumpState::INIT:

        if (lastState != currentState)
        {
            commonStateLogic();

            if(!cistern.toF_ready())
                cistern.setupToF();
        }

        /*
        if(pumpButton)
        {
            errorCode = 3;
            currentState = PumpState::ABORT;
        }
        */

        // Do as many checks as possible already in Irrigation class, dont enter submachine
        if (utils.countTime(stateBeginMillis, minStateDuration))
        {
            if (!cistern.toF_ready())
            {
                if(transCount < maxSelfTrans)
                {
                    transCount++;
                    cistern.setupToF();
                    currentState = PumpState::INIT;
                }
                else if(!cistern.validLiquidLevel(instr->allocatedWater))
                {
                    errorCode = 2;
                    currentState = PumpState::ABORT;
                }
                else
                {
                    errorCode = 1; // all setup attemps failed
                    currentState = PumpState::ABORT;
                }  
            }
            else
            {
                errorCode = 0; // Ok
                currentState = PumpState::ON;
                cistern.updateLiquidAmount();
            }
        }

        lastState = PumpState::INIT;
        break;

    case PumpState::ON:

        // entry Func
        if (lastState != currentState)
        {
            commonStateLogic();
            // cistern.readToF_cont();

            cistern.driveSolenoid(instr->solenoidValve, LOW);
        }

        // do / State Function
        switchOn();

        // Periodically measure waterFlow and powerUsage
        currentTime = millis();
        if (currentTime >= (lastTime + measureIntervall))
        {
            lastTime = currentTime;

            flow->measureFlow();
            flow->writePoint();

            // powerMeter1.measure();
            // powerMeter1.writePoint();
            powerMeter1.measureAndSubmit();
        }

        /*
        // Turn off components, write Data
        if(pumpButton)
        {
            errorCode = 3;
            currentState = PumpState::DONE;
        }
        */

        // exit Function
        // pumpTime up or User cancels Process
        if (utils.countTime(stateBeginMillis, instr->pumpTime))
        {
            currentState = PumpState::DONE;
        }

        lastState = PumpState::ON;
        break;

    case PumpState::DONE:

        if (lastState != currentState)
        {
            commonStateLogic();

            switchOff();

            cistern.driveSolenoid(instr->solenoidValve, HIGH);

            flow->measureAmount();

            // setup toF correctly, or crash
            instr->distributedWater = cistern.getLiquidPumped();

            cistern.updateLiquidAmount();
        }

        if (utils.countTime(stateBeginMillis, minStateDuration))
        {
            isDone = true;
        }

        lastState = PumpState::DONE;
        break;

    case PumpState::ABORT:
        if (lastState != currentState)
        {
            commonStateLogic();

            printError();
        }

        if (utils.countTime(stateBeginMillis, minStateDuration))
        {
            isDone = true;
        }

        lastState = PumpState::ABORT;
        break;
    }
}

/*
Details ledcWrite() for PWM (DC-Motors or LEDs):
https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html
https://diyi0t.com/arduino-pwm-tutorial/
*/
void Pump::switchOn()
{
    /*
    // H-Bridge, set Motor/Pump direction
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    */

    // Esp32
    ledcWrite(instr->pwmChannel, 255);

    // Esp8266
    // analogWrite(enA, 125);
}

void Pump::switchOff()
{
    ledcWrite(instr->pwmChannel, 0);
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

/*
final State is reached AND minStateTime is up (enough time for waterManagement())
*/
bool Pump::machineDone()
{
    // return lastState == PumpState::DONE || lastState == PumpState::ABORT;
    return isDone == true;
}

/*
Reset Machine for next run
lastState != currentState to run INIT entry Func once
*/
void Pump::resetMachine()
{
    isDone = false;
    currentState = PumpState::INIT;
    lastState = (PumpState)-1;
}

void Pump::printError()
{
    Serial.println(errors[errorCode]);
}

/*
lastState = (PumpState)-1;
Enums cannot be set to null, initial Value is always 0 aka PumpState.IDLE
Set to -1 so lastState != currentState, to run entry Func

Problem: cistern.toF.Status != VL53L0X_ERROR_NONE is true, even if not setup correctly
-> Crash when Measuring if not also toF.ready checked
*/

/*
Check already in Irrigation class, dont run submachine:

// solenoid not found / pumpmodel not active
else if(solenoidValve == -1) 
{
    errorCode = 5;
    currentState = PumpState::ABORT;
}
// solenoid reached waterLimit
else if(!Irrigation::validSolenoid(relaisChannel, Irrigation::waterLimit24h, 24))
{
    // Check waterLimits per SolenoidValve (on Button Press by User)
    // Data needs to be provided first (Utilities::provideData())
    errorCode = 3;
    currentState = PumpState::ABORT;
}
*/