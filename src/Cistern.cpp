#include "Cistern.h"

/*
Cistern::Cistern(int toF_address)
{
  this->toF_address = toF_address;
}
*/

bool Cistern::setupToF(int toF_address)
{
    /*
    Only rerun setup on first time or if sensor status suggests, see library
    Adafruit Library, on i2c bus with TSL2591, Error -5,
    works on same i2c bus with other VL530X with immediat shutPin LOW in main.setup()
    */

    if(toF_address == 0x51)
    {
        digitalWrite(shut_toF, LOW);
        delay(20);
        digitalWrite(shut_toF, HIGH);
        delay(20);
    }

    if (toF_ready == false || (toF.Status != VL53L0X_ERROR_NONE))
    {
        if (!toF.begin(toF_address, &I2Cone)) // &I2Cone
        {
            Serial.println(F("Failed to boot toF_2"));
            toF_ready = false;
            return false;
        }
    }
    toF_ready = true;
    return true;
}

bool Cistern::checkWaterLevel()
{
    float waterLevel = evaluateToF();
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

void Cistern::updateWaterLevel()
{
    int oldWaterDistance = currWaterDist;
    currWaterDist = evaluateToF();
    int pumpedWaterMM = oldWaterDistance - currWaterDist;
    int pumpedWaterML = pumpedWaterMM * 100;

    p2.clearTags();
    p2.clearFields();
    // p2.addTag("Plant Group", plantGroup);
    p2.addField("pumpedWaterMM", pumpedWaterMM);
    p2.addField("pumpedWaterML", pumpedWaterML);
    influxHelper.writeDataPoint(p2);
}

void Cistern::readToF(Adafruit_VL53L0X toF, int distances[])
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
    // Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds());
    // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);

    // Do n-valid Readings
    // 3 Attemps per single valid Reading (or e.g. time limit while in IDLE State), then stop
    // Out of Range: measure.RangeStatus == 4
    int samples = 8;
    VL53L0X_RangingMeasurementData_t measure;

    for (int i = 0; i < samples; i++)
    {
        int distance = 0;
        bool validReading = false;
        int j = 0; // 3 Attemps per single Reading

        while (j < 3 && !validReading)
        {
            // Water Container Physical Limits, maxPossibleWaterDist, ...
            if ((measure.RangeStatus == 4 || distance < minWaterDist || distance > maxWaterDist)) 
            {
                // toF_1.rangingTest(&measure, false);
                // distance = measure.RangeMilliMeter;
                distance = toF.readRange();
                j++;
            }
            else
            {
                validReading = true;
                // Serial.print(distance);
                // Serial.print(" ");
                distances[i] = distance;
            }
            delay(200);
        }
    }
}

float Cistern::evaluateToF() // Adafruit_VL53L0X toF
{
    float median, avgDistance;
    float tolerance = 5.0f;
    int totalSamples = 8;
    int validSamples = totalSamples;
    int distances[totalSamples];

    readToF(toF, distances);
    delay(200);

    for(int i = 0; i < totalSamples; i++)
    {
        median += distances[i];
    }

    median = median / totalSamples;

    for(int i = 0; i < totalSamples; i++)
    {
        // e.g. avgDistance 80mm, two invalid Readings 74mm, 86mm
        if(abs(median - distances[i]) > tolerance)
        {
            distances[i] = -1;
            validSamples -= 1;
        }
        else
        {
            avgDistance += distances[i];
        }
        Serial.print(distances[i]);
        Serial.print(" ");
    }

    avgDistance = avgDistance / validSamples;

    return avgDistance;
}

float Cistern::readToF_cont()
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);

    std::vector<int> distances;
    int i = 0;
    float sum = 0.0f;
    int sampleNum = 8;

    toF.startRangeContinuous(100);
    while (i < sampleNum)
    {
        int distance = toF.readRange();
        sum += distance;
        distances.push_back(distance);
        Serial.print(distance);
        Serial.print(" ");
        if (toF.timeoutOccurred())
        {
            Serial.print(" TIMEOUT");
        }
        i++;
    }

    toF.stopRangeContinuous();

    float avgDistance = sum / (distances.size() * 1.0f);
    Serial.print("Avg: ");
    Serial.println(avgDistance);
    return avgDistance;
}