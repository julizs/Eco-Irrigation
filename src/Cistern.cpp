#include "Cistern.h"

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

void Cistern::setupToF()
{
    if (toF_address == 0x51)
    {
        shutToF();
        delay(100);
    }
    if (!toF.begin(toF_address, &I2Cone))
    {
        Serial.println("Failed to boot toF at" + toF_address);
    }
    /*
    Error Codes see Strg + Click toF.Status, VL53L0X_ERROR_NONE
    Two VL530X on same i2C bus possible with immediate shut LOW in main.setup()
    toF_ready needed for first Setup
    */

   /*
    VL53L0X_Error status;
    VL53L0X_RangingMeasurementData_t measure;
    int attempt = 0;

    while (status != 0 && attempt < 3)
    {
        if (toF_address == 0x51)
        {
            shutToF();
            delay(100);
        }
        if (!toF.begin(toF_address, &I2Cone))
        {
            Serial.println("Failed to boot toF at" + toF_address);
        }
        delay(100);
        status = toF.rangingTest(&measure, false);
        attempt++;
    }

    return true;
   */
}

void Cistern::shutToF()
{
    digitalWrite(toF_shut, LOW);
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
void Cistern::updateIrrigations(uint8_t relaisChannel)
{
    int oldWaterDist = currWaterDist;
    int oldWaterAmount = calcMl(oldWaterDist);
    int newWaterDist = updateWaterLevel();
    int newWaterAmount = calcMl(newWaterDist);
    // Not less than 0ml
    // int pumpedWaterML = min(oldWaterAmount - newWaterAmount,0);
    int pumpedWaterML = rand() % 200 + 100; // Demo, 100-300ml

    // Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
    // Add affected Plants, SolenoidValve/relaisChannel and Pump as Tags
    // MongoDb or InfluxDB ? Reason ? Pump ? Pump Time ?
    p2.clearTags();
    p2.clearFields();
    char solenoidValve[4];
    itoa(relaisChannel, solenoidValve, 10);
    p2.addTag("solenoidValve", solenoidValve);
    p2.addField("pumpedWaterML", pumpedWaterML);
    influxHelper.writeDataPoint(p2);
}

/*
Only way to prevent Crash if one or both ToF setup fail:
Do Test Reading and only then compare err Code
*/
void Cistern::readToF(int distances[])
{
    VL53L0X_Error status;
    VL53L0X_RangingMeasurementData_t measure;
    toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY); // 200ms
    // Serial.println(toF_1.getMeasurementTimingBudgetMicroSeconds());
    // toF_1.setMeasurementTimingBudgetMicroSeconds(300000);

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

            if (measure.RangeStatus == 4 || distance > cisternHeight || distance <= 0)
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
    char message[50];

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

    sprintf(message, "VL530X 0x%x, Median: %0.2f", toF_address, median);
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