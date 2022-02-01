#include "Pump.h"

Pump::Pump(PumpModel &p) : pumpModel(p)
{
    setup();
}

void Pump::setup()
{
    //pinMode(enA, OUTPUT);
    //pinMode(in1, OUTPUT);
    //pinMode(in2, OUTPUT);

    minStateDuration = 4;
    minWaterDist = 50;
    maxWaterDist = 300;
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

bool Pump::setupToF_1()
{
    // Only if SHT_Pin connected to toF sensor
    // pinMode(shut_toF, OUTPUT);
    // digitalWrite(shut_toF, LOW);
    // digitalWrite(shut_toF, HIGH);

    // Only rerun setup on first time or if sensor satus suggests, see library
    if (toF_1_ready == false || (toF_1.Status != VL53L0X_ERROR_NONE))
    {
        if (!toF_1.begin(0x52, &I2Cone))
        {
            Serial.println(F("Failed to boot toF_1"));
            toF_1_ready = false;
            return false;
        }
    }
    toF_1_ready = true;
    return true;
}

bool Pump::setupToF_2()
{
    // Adafruit Library, on i2c bus with TSL2591, Error -5,
    // works on same i2c bus with other VL530X with immediat shutPin LOW in main.setup()
    if (toF_2_ready == false || (toF_2.Status != VL53L0X_ERROR_NONE))
    {
        digitalWrite(shut_toF, LOW);
        delay(20);
        digitalWrite(shut_toF, HIGH);
        delay(20);

        if (!toF_2.begin(0x51, &I2Cone))
        {
            Serial.println(F("Failed to boot toF_2"));
            toF_2_ready = false;
            //return false;
        }
    }
    toF_2_ready = true;
    return true;
}

bool Pump::checkWaterLevel()
{
    float waterLevel = readToF(toF_1);
    currWaterDist = waterLevel;

    if (waterLevel < maxWaterDist) // Limits set by User, minWaterDist doesnt matter
    {
        Serial.print("WaterLevel is sufficient for Irrigation: ");
        return true;
    }
    else
    {
        Serial.println("WaterLevel is too low for Irrigation.");
        return false;
    }
}

float Pump::readToF(Adafruit_VL53L0X toF)
{
    // More consistent readings, but: Continous Readings Func outputs Out of Range
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
    // Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds());
    // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);

    // Do 2 valid Reading and only then calc Average
    // 3 Attemps per single valid Reading (or e.g. time limit while in IDLE State), then stop
    // measure.RangeStatus == 4 means Out of Range
    float avgDistance = 0;
    int sampleNum = 8;
    VL53L0X_RangingMeasurementData_t measure;

    for (int i = 0; i < sampleNum; i++)
    {
        int distance = 0;
        bool validReading = false;
        int j = 0; // 3 Attemps per single Reading

        while (j < 3 && !validReading)
        {
            if ((measure.RangeStatus == 4 || distance < minWaterDist || distance > maxWaterDist)) // Water Container Physical Limits, maxPossibleWaterDist, ...
            {
                //toF_1.rangingTest(&measure, false);
                //distance = measure.RangeMilliMeter;
                distance = toF.readRange();
                j++;
            }
            else
            {
                validReading = true;
                avgDistance += distance;
                Serial.print(distance);
                Serial.print(" ");
            }
            delay(200);
        }
    }

    avgDistance /= (sampleNum * 1.0f);
    Serial.print("Avg: ");
    Serial.println(avgDistance);

    return avgDistance;
}

float Pump::readToF_cont()
{
    toF_1.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);

    std::vector<int> distances;
    int i = 0;
    float sum = 0.0f;
    int sampleNum = 8;

    toF_1.startRangeContinuous(100);
    while (i < sampleNum)
    {
        int distance = toF_1.readRange();
        sum += distance;
        distances.push_back(distance);
        Serial.print(distance);
        Serial.print(" ");
        if (toF_1.timeoutOccurred())
        {
            Serial.print(" TIMEOUT");
        }
        i++;
    }

    toF_1.stopRangeContinuous();

    float avgDistance = sum / (distances.size() * 1.0f);
    Serial.print("Avg: ");
    Serial.println(avgDistance);
    return avgDistance;
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
            toF_1_ready = setupToF_1();
        }

        //readToF_cont();

        if (millis() - stateBeginMillis >= minStateDuration * 1000UL && wateringNeeded)
        {
            if (toF_1_ready)
            {
                if (checkWaterLevel()) // Update both currWaterDist and check if valid with same 1 Reading
                {
                    currentState = PumpState::ON;
                    //toF_1_ready = false; // For next iteration
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
                toF_1_ready = setupToF_1();
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

            wateringNeeded = false;
            switchOff();

            int oldWaterDistance = currWaterDist;
            currWaterDist = readToF(toF_1);
            int pumpedWater = oldWaterDistance - currWaterDist;
            // Send Values in InfluxDB for Persistence
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
    //analogWrite(enA, 125);

    // Esp32
    // Run Pump1 with 5V (12V Input L298N, 10V Output at max. PWM Duty)
    ledcWrite(pwmChannel1, 255); // Palermo
    ledcWrite(pwmChannel2, 255); // QR50E

    //digitalWrite(pumpPin, HIGH); // via Relais
}

void Pump::switchOff()
{
    //digitalWrite(pumpPin, LOW); // via Relais
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