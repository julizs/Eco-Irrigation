#include "Pump.h"

Pump::Pump(PumpModel &p) : pumpModel(p)
{
    setup();
    // Cistern cistern1(0x51);
}

void Pump::setup()
{
    // pinMode(enA, OUTPUT);
    // pinMode(in1, OUTPUT);
    // pinMode(in2, OUTPUT);

    minStateDuration = 4;
    currentState = PumpState::IDLE;

    setupPWM();
}

void Pump::setupPWM()
{
    // Setup 2 PWM Channels for Pumps
    // https://randomnerdtutorials.com/esp32-pwm-arduino-ide/
    // https://diyi0t.com/arduino-pwm-tutorial/
    pinMode(pumpPWM_Pin_1, OUTPUT);
    pinMode(pumpPWM_Pin_2, OUTPUT);
    ledcSetup(pwmChannel1, freq, res); // Channel, Frequency, Resolution (8 Bit, 0-255)
    ledcSetup(pwmChannel2, freq, res);
    ledcAttachPin(pumpPWM_Pin_1, pwmChannel1);
    ledcAttachPin(pumpPWM_Pin_2, pwmChannel2);
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
    // Constrain to 0 - 10 Ampere, otherwise infinity bug when on USB, InfluxDB fail to write
    data.current = abs(constrain(ina219.getCurrent_mA(), 0, 10000) / 1000.0f);
    // power_mW = ina219.getPower_mW();
    data.power = data.current * data.busVoltage;

    return data;
}

/*
IrrSystem Algo decides PumpTime according to Mililiters
and Info from Pump Model (PWM, FlowRate etc.)
1 State Machine loop per Irrigation
irrigationSubject can be a plant or plantGroup
*/
void Pump::prepareIrrigation(const char *irrigationSubject, int irrigationAmount)
{
    // 2. Get Solenoid (Relais Channel), SolenoidState (Relais State) and Pump Name
    char url[50] = "";
    strcat(url, baseUrl);
    strcat(url, "/solenoidValves");
    DynamicJsonDocument solenoids = services.doJSONGetRequest(url);

    bool relaisOpen;

    // For every Solenoid
    for (int i = 0; i < solenoids.size(); i++)
    {
        const char *plantGroupOpen = solenoids[i]["open"];
        const char *plantGroupClosed = solenoids[i]["closed"];

        // Check if PlantGroup is attached to this Solenoid (and which State)
        if (strcmp(plantGroupOpen, irrigationSubject) == 0)
        {
            relaisOpen = true;
        }
        else if (strcmp(plantGroupClosed, irrigationSubject) == 0)
        {
            relaisOpen = false;
        }
        else
        {
            continue; // Skip rest of Code, goto next Solenoid
        }

        const char *pumpName = solenoids[i]["pump"];

        // Per Plant Group:
        // 1. TODO Drive Relais
        int relaisChannel = solenoids[i]["relais"].as<int>() - 1;
        Serial.println(relaisChannel); Serial.println(relaisOpen);

        // 2. Drive Pump
        doIrrigation(pumpName, irrigationAmount);
    }
}

void Pump::doIrrigation(const char* pumpName, int irrigationAmount)
{
    char url[50] = "";
    strcat(url, baseUrl);
    strcat(url, "/pumps/");
    strcat(url, pumpName);
    DynamicJsonDocument pump = services.doJSONGetRequest(url);

    int litersPerHour = pump["flowRate"][1].as<int>();
    int pwmChannel = pump["pwmChannel"].as<int>();
    if (litersPerHour == 0)
    {
        // avoid DivideByZero
        litersPerHour = 300;
    }
    float pumpTime = (irrigationAmount / (litersPerHour * 1000.0f)) * 3600;

    // Serial.print("Irrigate: "); Serial.println(plantGroup);
    Serial.println(pwmChannel); Serial.println(pumpTime);
    
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

            // Check if ToF Sensor of Cistern (containing this Pump) is ready
            cistern1.toF_ready = cistern1.setupToF(0x51);
        }

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && wateringNeeded)
        {
            if (cistern1.toF_ready)
            {
                if (cistern1.checkWaterLevel()) // Update both currWaterDist and check if valid with same 1 Reading
                {
                    currentState = PumpState::ON;
                    // toF_1_ready = false; // For next iteration
                }
                else
                {
                    Serial.println("Not enough Water, skipping Irrigation.");
                    currentState = PumpState::DONE;
                }
            }
            else
            {
                // Try to setup again
                cistern1.toF_ready = cistern1.setupToF(0x51);
                delay(100);
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
        }

        // Execute each tick
        // 2 Checks for Safety: Water Distance and Max. Pump Time
        switchOn();

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL &&
            ((millis() - stateBeginMillis >= pumpModel.maxPumpingDuration * 1000UL) || wateringNeeded == false)) // minStateTime is up AND (Time is up OR Button pressed again (Stop now!))
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

            INAdata inaData = readIna();
            p0.clearFields();
            p0.addField("voltage", inaData.voltage);
            p0.addField("current", inaData.current);
            p0.addField("power", inaData.power);
            p0.addField("busVoltage", inaData.busVoltage);
            p0.addField("shuntVoltage", inaData.shuntVoltage);

            delay(100);

            wateringNeeded = false;

            switchOff();

            // Do InfluxDB Updates AFTER turning off pump
            cistern1.updateWaterLevel();

            influxHelper.writeDataPoint(p0);  
        }

        lastState = PumpState::DONE;
        break;
    }
}

/*
bool Pump::checkPumpPerformance(unsigned short lastPumped)
{
    // Correct: bestPumped struct per Model and Voltage or PWM value (e.g. Palermo, 100ml, 125)
    if(bestPumped - lastPumped > 50) // 50 Milliliter Threashold
    {
        Serial.println("Pump Performance deteriorated, need to clean Pump Filter or Exchange Water.");
        return true;
    }
    return false;
}
*/

/*
bool Pump::sufficientWaterLevel()
{
    //return toF_1.readRangeSingleMillimeters() > maxWaterDistance;
}

float Pump::currentwaterLevel()
{
    if(checkMinWaterDistance())
    {
        Serial.println("Water Level not sufficient. Please refill Tank.");
    }
    //return (distanceDeltaToMilliliters(toF.readRangeSingleMillimeters()));
}
*/

void Pump::switchOn()
{
    /*
    // Set Rotation Direction (only if H-Bridge and Pump supports 2 Directions)
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    */

    // Esp8266
    // analogWrite(enA, 125);

    // Esp32
    // Run Pump1 with 5V (12V Input L298N, 10V Output at max. PWM Duty)
    ledcWrite(pwmChannel1, 255); // Palermo
    ledcWrite(pwmChannel2, 255); // QR50E

    // digitalWrite(pumpPin, HIGH); // via Relais
}

void Pump::switchOff()
{
    // digitalWrite(pumpPin, LOW); // via Relais
    ledcWrite(pwmChannel1, 0); // Palermo
    ledcWrite(pwmChannel2, 0); // QR50E
}

const Pump::PumpModel &Pump::getPumpModel() const
{
    return pumpModel;
}

void Pump::setPumpModel(const Pump::PumpModel &pM)
{
    pumpModel = pM;
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