#include "Cistern.h"

Cistern::Cistern(int toF_address)
{
    this->toF_address = toF_address;
    minWaterDist = 50;
    maxWaterDist = 300;
    sampleSize = 8;
}

bool Cistern::setupToF()
{
    /*
    Error Codes see Strg + Click toF.Status, VL53L0X_ERROR_NONE
    Two VL530X on same i2C bus possible with immediate shut LOW in main.setup()
    */
    if (toF_ready == false || (toF.Status != 0)) // 0 = VL53L0X_ERROR_NONE
    {
        if (!toF.begin(toF_address, &I2Cone)) // &I2Cone
        {
            Serial.println(F("Failed to boot toF at" + toF_address));
            toF_ready = false;
            return false;
        }
    }
    toF_ready = true;
    return true;
}

void Cistern::shutToF()
{
    digitalWrite(toF_shut, LOW);
    delay(50);
    digitalWrite(toF_shut, HIGH);
    delay(50);
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
    
    return true;
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

void Cistern::readToF(int distances[])
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
    // Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds());
    // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);

    // Do n-valid Readings
    // 3 Attemps per single valid Reading (or e.g. time limit while in IDLE State), then stop
    // Out of Range: measure.RangeStatus == 4
    VL53L0X_RangingMeasurementData_t measure;

    for (int i = 0; i < sampleSize; i++)
    {
        int distance = 0;
        bool validReading = false;
        int j = 0; // 3 Attemps per single Reading

        while (j < 3 && !validReading)
        {
            // Water Container Physical Limits, maxPossibleWaterDist, ...
            if ((measure.RangeStatus == 4 || distance < minWaterDist || distance > maxWaterDist))
            {
                // toF.rangingTest(&measure, false);
                // distance = measure.RangeMilliMeter;
                distance = toF.readRange();
                if(toF.timeoutOccurred()) 
                { 
                    Serial.print("VL530X Timeout");
                    delay(50);
                    continue;
                }
                j++;
            }
            else
            {
                validReading = true;
                distances[i] = distance;
            }
            delay(200);
        }
    }
    Serial.println();
}

float Cistern::evaluateToF() // Adafruit_VL53L0X toF
{
    float median, avgDistance;
    float tolerance = 5.0f;
    int validSamples = sampleSize;
    int distances[sampleSize];

    readToF(distances); // Manipulate Array
    delay(200);

    Serial.print("Measurement VL530X at Address ");
    Serial.print(toF_address == 0x51 ? "0x51" : "0x52");
    Serial.print(": ");
    Serial.println();

    for (int i = 0; i < sampleSize; i++)
    {
        median += distances[i];
        Serial.print(distances[i]);
        Serial.print(" "); 
    }
    Serial.println();

    median = median / sampleSize;

    for (int i = 0; i < sampleSize; i++)
    {
        // e.g. avgDistance 80mm, two invalid Readings 74mm, 86mm
        if (abs(median - distances[i]) > tolerance)
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
    Serial.println();
    Serial.print("Median: ");
    Serial.print(median);
    Serial.println();

    if(validSamples > 0) // Avoid Divide by 0
    {
        avgDistance = avgDistance / validSamples;
    }
    else
    {
        avgDistance = -1;
    }
    
    Serial.print("Avg: ");
    Serial.print(avgDistance);
    Serial.println();

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