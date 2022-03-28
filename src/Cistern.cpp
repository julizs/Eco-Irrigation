#include "Cistern.h"

Cistern::Cistern(unsigned short toF_address, unsigned short cisternHeight, float mmToMl)
{
    this->toF_address = toF_address;
    this->cisternHeight = cisternHeight;
    this->mmToMl = mmToMl;
    currWaterDist = 0;
    minWaterDist = 50;
    maxWaterDist = 300;
    sampleSize = 8;
}

void Cistern::setupToF()
{
    /*
    Error Codes see Strg + Click toF.Status, VL53L0X_ERROR_NONE
    Two VL530X on same i2C bus possible with immediate shut LOW in main.setup()
    toF_ready needed for first Setup
    */
    int attempts = 0;

    if (toF_ready == false || toF.Status != VL53L0X_ERROR_NONE)
    {
        while (attempts < 4)
        {
            if (toF_address == 0x51)
            {
                shutToF();
                delay(100);
            }
            if (!toF.begin(toF_address, &I2Cone))
            {
                Serial.println("Failed to boot toF at" + toF_address);
                toF_ready = false;
                attempts++;
            }
            else
            {
                // ToF is ready
                toF_ready = true;
                return;
            }
            // delay(500);
        }
    }
}

void Cistern::shutToF()
{
    digitalWrite(toF_shut, LOW);
    delay(50);
    digitalWrite(toF_shut, HIGH);
    delay(50);
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

        char key1[25], key2[25];
        sprintf(key1, "waterDistance 0x%x", this->toF_address);
        p0.addField(key1, currWaterDist);
        sprintf(key2, "waterAmount 0x%x", this->toF_address);
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

// Point p2
void Cistern::updateIrrigations()
{
    int oldWaterDist = currWaterDist;
    int oldWaterAmount = calcMl(oldWaterDist);
    int newWaterDist = updateWaterLevel();

    int newWaterAmount = calcMl(newWaterDist);
    int pumpedWaterML = oldWaterAmount - newWaterAmount;
    // Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
    // Reason ? Pump ? Pump Time ? MongoDb instead of InfluxDB ?
    p2.clearTags();
    p2.clearFields();
    p2.addTag("Plant", "Magnolie");
    p2.addField("pumpedWaterML", pumpedWaterML);
    influxHelper.writeDataPoint(p2);
}

void Cistern::readToF(int distances[])
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // 200ms
    // Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds());
    // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);

    // Do n-valid Readings
    // 3 Attemps per Reading (or > IDLE State time limit)
    VL53L0X_RangingMeasurementData_t measure;

    for (int i = 0; i < sampleSize; i++)
    {
        int distance = 0;
        bool validReading = false;
        int j = 0; // 3 Attemps per Reading

        while (!validReading && j < 3)
        {
            VL53L0X_Error err = toF.rangingTest(&measure, false);
            // while(toF.waitRangeComplete()) {}

            if(err == VL53L0X_ERROR_NONE)
            {
                distance = measure.RangeMilliMeter;
            }
            
            // distance = toF.readRange();

            // Out of Range or Water Tank Physical Limits
            if (measure.RangeStatus == 4 || distance > cisternHeight || distance <= 0)
            {
                validReading = false;
                j++;   
            }
            else
            {
                validReading = true;
                distances[i] = distance;
            }
            // delay(200);
        }
    }
}

/*
Sort out the worst Readings
e.g. avgDistance 80mm, two invalid Readings 77mm, 83mm
*/
float Cistern::evaluateToF() // Adafruit_VL53L0X toF
{
    float distancesSum = 0.0f;
    int tolerance = 2; // mm
    int validSamples = sampleSize;
    int distances[sampleSize];
    char message[50];

    readToF(distances); // Manipulate Array

    sprintf(message, "Measure VL530X at: 0x%x", toF_address);
    Serial.println(message);

    for (int i = 0; i < sampleSize; i++)
    {
        distancesSum += distances[i];
        Serial.print(distances[i]);
        Serial.print(" ");
    }
    Serial.println();

    float tempAvg = distancesSum / (sampleSize * 1.0f);

    for (int i = 0; i < sampleSize; i++)
    {
        if (abs(tempAvg - distances[i]) > tolerance)
        {
            distancesSum -= distances[i];
            distances[i] = -1;
            validSamples -= 1;
        }

        Serial.print(distances[i]);
        Serial.print(" ");
    }
    Serial.println();

    float finalAvg;
    if (validSamples > 4)
    {
        finalAvg = distancesSum / (validSamples * 1.0f);
    }
    else
    {
        finalAvg = 0;
    }

    sprintf(message, "TempAvg: %0.2f, FinalAvg: %0.2f", tempAvg, finalAvg);
    Serial.println(message);

    return finalAvg;
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
        //distances.push_back(distance);
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
    // float avgDistance = sum / (distances.size() * 1.0f);
}