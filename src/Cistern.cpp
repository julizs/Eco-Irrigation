#include "Cistern.h"

/*
Measurements:
5 Liters = 83mm from Table = 135mm Distance from Sensor
4 Liters = 146mm 
3 Liters = 156mm
2 Liters = 167mm (lower LIMIT)
0,1 Liter: = 179mm (Unsafe for Pump)
Rule: About 1cm per Liter, -1mm the higher it goes (Cistern gets wider)
-> Pumping: 1 Liter = 10mm, 500ml = 5mm
-> Max(Safe): 9 Liters, Min (Safe): 2 Liters
*/

Cistern::Cistern(uint8_t toF_address, uint8_t relaisChannels[], int cisternHeight, float mmToMl)
{
    this->toF_address = toF_address;
    this->cisternHeight = cisternHeight;
    this->mmToMl = mmToMl;

    solenoidValves = relaisChannels;
    currWaterDist = 0;
    minWaterDist = 50;
    maxWaterDist = 200;
    sampleSize = 8;
}

/*
Wrong Readings if spadCount after (re)Setup is different (e.g. 3 instead of 4 spads)
(Check while sensor debug mode true)
https://wolles-elektronikkiste.de/vl53l0x-und-vl53l1x-tof-abstandssensoren
toF.status is 0 even if not correctly setup
toF.rangingTest for current ErrCode crashes if toF not setup before
*/

bool Cistern::setupToF()
{
    VL53L0X_Error status;
    VL53L0X_RangingMeasurementData_t measure;
    int attempt = 0;

    // Serial.println(toF.Status);
    // true/false for debug infos while Setup
    while (!toF.begin(toF_address, false, &I2Cone) && attempt < 3)
    {
        delay(1000 * attempt);
        Serial.println(toF.Status);
        attempt++;
        if (toF_address == 0x51)
        {
            shutToF();
        }
    }
    // Serial.println(toF.Status);
    
    if(toF.Status == 0)
    {
        toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // 200ms
        // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);
        toF_ready = true;
    }
    else
    {
        toF_ready = false;
        // critErrMessage = "Final fail to setup ToFs.";
        critErrCode = 3;
    }

    return true;
}
/*
bool Cistern::setupToF()
{
    VL53L0X_Error status;
    VL53L0X_RangingMeasurementData_t measure;
    int attempt = 0;

    // true/false for debug infos while Setup
    while (attempt < 5)
    {
        stateBeginMillis = millis();
        if (toF_address == 0x51)
        {
            shutToF();
            delay(50);
        }
        if (!toF.begin(toF_address, true, &I2Cone))
        {
            while (!countTime(2));
            // delay(500 * attempt);
        }
        else 
        { return true; }
        attempt++;
        Serial.println(toF.Status);
    }
}
*/

void Cistern::shutToF()
{
    // Serial.println("Shutting");
    digitalWrite(toF_shut, LOW);
    delay(10);
    digitalWrite(toF_shut, HIGH);
}

bool Cistern::validWaterLevel()
{
    return currWaterDist < maxWaterDist;
}

// Point p0
int Cistern::updateWaterLevel()
{
    int measurement = evaluateToF();

    // Only create new Datapoint if Measurement is valid,
    // else keep old Datapoint as current
    if (measurement != 0)
    {
        currWaterDist = measurement;

        char key1[32], key2[32];
        snprintf(key1, 32, "waterDistance0x%x", this->toF_address);
        p0.addField(key1, currWaterDist);
        snprintf(key2, 32, "waterAmount0x%x", this->toF_address);
        p0.addField(key2, calcMl(currWaterDist));
    }

    return currWaterDist;
}

int Cistern::calcMl(float waterDistance)
{
    // Consider Tank getting thinner/thicker
    int waterAmount = waterDistance * mmToMl;
    return waterAmount;
}

/*
Point p2
sendReport replaces updateIrrigations
void Cistern::updateIrrigations(uint8_t relaisChannel)
{
    int oldWaterDist = currWaterDist;
    int oldWaterAmount = calcMl(oldWaterDist);
    int newWaterDist = updateWaterLevel();
    int newWaterAmount = calcMl(newWaterDist);
    // int pumpedWaterML = min(oldWaterAmount - newWaterAmount,0); // Not less than 0ml
    int pumpedWaterML = rand() % 200 + 100; // Demo, 100-300ml

    // Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
    p2.clearTags();
    p2.clearFields();
    char solenoidValve[4];
    itoa(relaisChannel, solenoidValve, 10);
    p2.addTag("solenoidValve", solenoidValve);
    p2.addField("pumpedWaterML", pumpedWaterML);
    influxHelper.writeDataPoint(p2);
}
*/

/*
Only way to prevent Crash if one or both ToF setup fail:
Do Test Reading and only then compare err Code
*/
void Cistern::readToF(int distances[])
{
    // Do n-valid Readings, 3 Attemps (j) per Reading
    for (int i = 0; i < sampleSize; i++)
    {
        uint16_t distance = 0;
        bool validReading = false;
        int j = 0;

        while (!validReading && j < 3)
        {
            // status = toF.rangingTest(&measure, false);
            // distance = measure.RangeMilliMeter;
            distance = toF.readRange();
            // while(toF.waitRangeComplete()) {}

            if (distance > cisternHeight || distance <= 0)
            {
                validReading = false;
                j++;
            }
            else
            {
                validReading = true;
                distances[i] = distance;
                Serial.print(distance);
                Serial.print(" ");
            }
            // delay(200);
        }
    }
}

/*
Use Median / 50th Percentile of Readings
*/
float Cistern::evaluateToF()
{
    int n = sampleSize;
    // int n = sizeof(distances) / sizeof(distances[0]);
    int distances[n];
    float median = 0.0f;
    char message[64];

    readToF(distances); // Pass by Reference
    std::sort(distances, distances + n);

    /*
    for (int i = 0; i < sampleSize; i++) {
       Serial.print(distances[i]); Serial.print(" ");
    }
    */

    if (n % 2 != 0)
    {
        median = (float)distances[n / 2];
    }
    else
    {
        median = (float)(distances[(n - 1) / 2] + distances[n / 2]) / 2.0;
    }

    snprintf(message, 64, "VL530X 0x%x, Median: %0.2f", toF_address, median);
    Serial.println(message);

    return median;
}

void Cistern::readToF_cont(int distances[])
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);

    // std::vector<int> distances;
    int i = 0;

    toF.startRangeContinuous(100);

    while (i < sampleSize)
    {
        int distance = toF.readRange();
        // distances.push_back(distance);
        distances[i] = distance;
        Serial.print(distance);
        Serial.print(" ");

        if (toF.timeoutOccurred())
        {
            Serial.print("ToF Timeout");
        }

        // isRangeComplete, waitRangeComplete ?
        i++;
    }

    toF.stopRangeContinuous();
}

void Cistern::driveSolenoid(uint8_t relaisChannel, uint8_t state)
{
    digitalWrite(Relais[relaisChannel], state);
}