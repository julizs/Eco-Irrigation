#include "Cistern.h"

Cistern::Cistern(unsigned short toF_address, unsigned short cisternHeight, float mmToMl)
{
    this->toF_address = toF_address;
    this->cisternHeight = cisternHeight;
    this->mmToMl = mmToMl;
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

    if(toF_ready == false || toF.Status != 0) // 0 = VL53L0X_ERROR_NONE
    {
        while (attempts < 4) 
        {
            if(toF_address == 0x51)
            {
                shutToF();
                delay(100);
            }
            if (!toF.begin(toF_address, &I2Cone)) // &I2Cone
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
            delay(1000);
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
    if(measurement != 0)
    {
        currWaterDist = evaluateToF();
        char key1[25], key2[25];
        sprintf(key1, "waterDistance %d", this->toF_address);
        p0.addField(key1, currWaterDist);
        sprintf(key2, "waterAmount %d", this->toF_address);
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
    int newWaterDist = updateWaterLevel();
    int deltaMM = 0;

    if(newWaterDist != 0) 
    {
        deltaMM = oldWaterDist - currWaterDist;
    }
 
    int pumpedWaterML = calcMl(deltaMM);

    // Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
    p2.clearTags();
    p2.clearFields();
    p2.addTag("Plant", "Magnolie");
    // Reason ? Pump ? Pump Time ? MongoDb instead of InfluxDB ?
    p2.addField("pumpedWaterMM", deltaMM);
    p2.addField("pumpedWaterML", pumpedWaterML);
    influxHelper.writeDataPoint(p2);
}

void Cistern::readToF(int distances[])
{
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
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
            // toF.rangingTest(&measure, false);
            // distance = measure.RangeMilliMeter;
            distance = toF.readRange();

            // Out of Range or Water Tank Physical Limits
            if (measure.RangeStatus == 4 || distance > cisternHeight || distance < 0)
            {         
                validReading = false;
                j++;
                
                /*
                if(toF.timeoutOccurred()) 
                { 
                    Serial.print("VL530X Timeout");
                    delay(50);
                    continue;
                }
                */         
            }
            else
            {
                validReading = true;
                distances[i] = distance;
            }

            delay(100);
        }
    }
}

float Cistern::evaluateToF() // Adafruit_VL53L0X toF
{
    float tempAvg, finalAvg;
    int tolerance = 5; // mm
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
        tempAvg += distances[i];
        Serial.print(distances[i]);
        Serial.print(" "); 
    }
    Serial.println();

    tempAvg = tempAvg / sampleSize;  

    for (int i = 0; i < sampleSize; i++)
    {
        // e.g. avgDistance 80mm, two invalid Readings 74mm, 86mm
        if (abs(tempAvg - distances[i]) > tolerance)
        {
            distances[i] = -1;
            validSamples -= 1;
        }
        else
        {
            finalAvg += distances[i];
        }
        
        Serial.print(distances[i]);
        Serial.print(" ");       
    }

    Serial.println();
    Serial.print("TempAvg: ");
    Serial.print(tempAvg);
    Serial.println();

    // > 0 Avoid Divide by 0
    // > 4 Minimum No. of valid Samples
    if(validSamples > 4) 
    {
        finalAvg = finalAvg / validSamples;
    }
    else
    {
        finalAvg = 0;
    }
    
    Serial.print("FinalAvg: ");
    Serial.print(finalAvg);
    Serial.println();

    return finalAvg;
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