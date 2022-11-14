#include "Pump.h"

// FlowMeter flow(flowPin);

// Add flowPin Number to Constr if multiple FlowMeters
Pump::Pump(Cistern &c) : cistern(c)
{
    flow = new FlowMeter(flowPin1);
    setup();
    setupPWM();
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

    lastState = (PumpState)-1;
    measureIntervall = 1000; // ms
}

// Setup ALL pwmPins for Pumps
void Pump::setupPWM()
{

    frequency = 30000;
    resolution = 8; // bits
    dutyCycle = 200;

    for(int i = 0; i < 2; i++) // numPumps
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

            // Apply Usersettings from REST (e.g. dutyCycle)
            // setup();

            if(!cistern.toF_ready())
                cistern.setupToF();
        }

        if (utils.countTime(stateBeginMillis, minStateDuration))
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
            else if(!cistern.validLiquidLevel(instr->allocatedWater))
            {
                errorCode = 3;
                currentState = PumpState::ABORT;
            }
            else
            {
                errorCode = 0; // None
                currentState = PumpState::ON;
                cistern.updateLiquidAmount();
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
            cistern.driveSolenoid(instr->solenoidValve, LOW);
        }

        // do / State Function
        switchOn();

        // Repeat Measurements in Intervall
        currentTime = millis();

        if (currentTime >= (lastTime + measureIntervall))
        {
            lastTime = currentTime;

            // Measure and writePoints

            // cistern.meter.measureFlow();
            flow->writePoint();

            // powerMeter1.measureIna();
            powerMeter1.measureAndSubmit();
        }

        /*
        // User presses HW-Button to cancel, measure Water
        if (pumpButton == true)
        {
            currentState = PumpState::DONE;
        }
        */

        // Check constantly
        if (utils.countTime(stateBeginMillis, minStateDuration) 
        && utils.countTime(stateBeginMillis, instr->pumpTime))
        {
            Serial.println(instr->pumpTime);
            Serial.println("Done");
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

            cistern.driveSolenoid(instr->solenoidValve, HIGH);

            flow->measureAmount();

            // Only measure if Water was pumped
            // toF must be setup correctly, or crash here
            cistern.updateLiquidPumped();
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
lastState must be != currentState for entry Logic to run once
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