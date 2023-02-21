#include "Cistern.h"

/*
1. Calc max. measurable Distance
maxDist = cisternHeight - sensorHeight - reflectorHeight
e.g. 210mm - 20mm - 20mm = 170mm

2. Get currDist, add sensorError

3. Calc delta from maxDist and currDist
currLevel =  maxDist - currDist
e.g. 170mm - 135mm + 20mm= 55mm
e.g. 170mm - 170mm  + 20mm = 20mm (min. reached)

4. Use max() to prevent negative Value caused by sensor inaccuracy
of failed irrigations (-1mm)
*/

/*
1. Manuelle Messungen:
9 Liters = 102mm
8 Liters = 110mm
7 Liters = 118mm
6 Liters = 127mm (90mm from Boxbottom)
5 Liters = 136mm
4 Liters = 146mm
3 Liters = 156mm
2 Liters = 167mm (minValid)
0,1 Liter: = 179mm (not changing)
Rule:
Rounded: 10mm/1cm per 1L -> 1mm per 100ml
Slope: -1mm per 2 Liters higher (Cistern gets wider)

2. Formeln:
Näherungsverfahren (ohne Integral) Keplersche Fassregel 
(nicht nötig für Frustum, da Spezialfall eines Prismatoids mit einfacher Berechnung)
ODER
Volumen of Frustum, Moscow Papyrus Problem 14:
https://en.wikipedia.org/wiki/Moscow_Mathematical_Papyrus

Kein Prisma sondern Prismatoid, da Grund- und Deckfläche nicht kongruent
Oder Pyramidenstumpf/ Frustum? (ähnl. Kegel Frustum)
Dugout / Lagoon / Trapezoid / Prismoid Tank Volume
https://www.onlineconversion.com/object_volume_trapezoid.htm
https://www.agric.gov.ab.ca/app19/calc/volume/dugout.jsp
https://www.lernhelfer.de/schuelerlexikon/mathematik/artikel/prismatoid#
http://www.solving-math-problems.com/volume-of-frustum-of-a-pyramid.html (*)
https://mathworld.wolfram.com/PyramidalFrustum.html (*)
https://www.tagblatt.de/Nachrichten/Das-Fass-und-seine-Formel-400021.html
https://mathepedia.de/Keplersche_Fassregel.html

1m³ = 1000L
1m² = 1.000.000mm²
*/

Cistern::Cistern(uint8_t toF_address)
{
    this->toF_address = toF_address;

    contents = LiquidType::WATER;

    minValidLevel = 40; // Height of Reflection Object
    maxDist = 190; // sensor Height above Cistern Bottom
    sensorError = -15; // mm
    sampleSize = 8;
}

/*
Only way to prevent Crash if one or both ToF setup fail:
Do Test Reading and only then compare err Code
Wrong Readings after resetup if spadCount different (e.g. 3 or 4 spads)
Check VL530X Library Memberfunctions and return Values
*/
void Cistern::setupToF()
{
    // Set debug mode to true for detailed infos
    toF.begin(toF_address, true, &I2Cone);
    delay(100);
    bool configDone = toF.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);

    if(toF.Status == VL53L0X_ERROR_NONE && configDone)
    {
        Serial.println("ToF setup correctly.");
    }
    else
    {
        Serial.println(((VL53L0X_Error)toF.Status));
    }
}

/*
Make test Measurement to obtain current toF.Status 
*/
bool Cistern::toF_ready()
{
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error error = toF.rangingTest(&measurement);

    Serial.print("ToF Test-Measurement: ");
    Serial.println(measurement.RangeMilliMeter);
    Serial.print("State: ");
    Serial.println(toF.Status);

    return toF.Status == 0;
}

void Cistern::shutToF()
{
    // Serial.println("Shutting");
    digitalWrite(toF_shut, LOW);
    delay(10);
    digitalWrite(toF_shut, HIGH);
}

uint16_t Cistern::getLiquidLevel()
{ 
    liquidDist = evaluateToF() + sensorError;
    liquidLevel = max((maxDist - liquidDist),0);

    if(liquidLevel <= minValidLevel)
        return 0;
    
    return liquidLevel;
}

uint16_t Cistern::getLiquidAmount()
{ 
    liquidAmount = calcMl(getLiquidLevel());

    Serial.print("Available Water (Ml): ");
    Serial.println(liquidAmount);

    return liquidAmount;
}

/*
Assert if waterLevel is valid/sufficient, by comparing available
liquidVolume in Cistern and allocatedLiquid of an Irrigation Event (automatic or manual)
(same procedure as validSolenoid)
*/
bool Cistern::validLiquidLevel(int allocatedLiquid = 0)
{
    bool isValid;

    liquidAmount = getLiquidAmount();
    uint16_t minValidVolume = calcMl(minValidLevel);

    if(allocatedLiquid > 0)
    {
        isValid = liquidAmount - allocatedLiquid >= minValidVolume ? true : false;
    }
    else
    {
        isValid = liquidAmount > minValidVolume;
    }

    // return currLiquidAmount - allocatedLiquid >= minValidLiquidAmount;
    return isValid;
}

/*
Only called by Pump::DONE
Calc Milliliters, write Environment_Data Point to Buffer, give Warnings (low waterLevel etc.)
10mm pumped delta can result in different waterAmounts, depending on waterLevel and cistern shape
-> Calc pumpedLiquid from liquidVolumes deltas, NOT liquidLevels deltas
*/
uint16_t Cistern::getLiquidPumped()
{
    int oldLiquidAmount = liquidAmount; // Calculated and submitted by PUMP::INIT
    liquidAmount = getLiquidAmount();

    // prevent undefined values for InfluxDB (measureError or Aborted)
    uint16_t pumpedLiquid = max((oldLiquidAmount - liquidAmount),0);

    return pumpedLiquid;
}

// Problem: clear point or not (called by Main::measure and Pump::done)
void Cistern::updateLiquidAmount()
{
    p0.clearTags();
    p0.clearFields();
    
    char key1[32], key2[32];

    // key is content of cistern (water, ph-agent, ...)
    snprintf(key1, 32, "%s_level", "water");
    p0.addField(key1, liquidLevel);
    snprintf(key2, 32, "%s_amount", "water");
    p0.addField(key2, liquidAmount);

    // Serial.println(p0.toLineProtocol());
    InfluxHelper::writeDataPoint(p0);
}

uint32_t Cistern::calcMl(int waterLevel)
{
    float l_B = 290, w_B = 220; // mm
    float l_T = 320, w_T = 250; // -> 3cm Slope per 20cm -> 1.5cm per 10cm
    float A_B = l_B * w_B; // Area Bottom: 0,0638m²
    // double A_T = l_T * w_T; // Area Top: 0.08m² (changes depending on fillLevel)
    // int h = 220;

    // Calc/Measure Slope
    // (1.5cm per 10cm, 15cm (0.15m) per 1m, 0.15cm (1.5mm) per 1cm, 0.15mm per 1mm)
    float sl_l = 0.15, sl_w = 0.15; // mm je mm
    
    // 1. Set Height
    uint16_t h = waterLevel;

    // 2. Slope in mm per m (for a given waterLevel):
    // e.g. 33mm * 0.15mm/mm = 4.95mm
    float sl_l_eff = h * sl_l;
    float sl_w_eff = h * sl_w;

    // 3. Calc A_T (new top area):
    // (Slope in both directions each -> +effective Slope * 2)
    l_T = l_B + sl_l_eff * 2;
    w_T = w_B + sl_w_eff * 2;
    uint32_t A_T = l_T * w_T;
    
    // 3. Calc Volume, Moscow Formula
    uint32_t waterVolume = h/3.0f * (A_B + A_T + sqrt(A_B*A_T));

    uint32_t waterAmount = waterVolume / 1000.0f; // mm³ /1000 = Ml

    return waterAmount;
}


void Cistern::readToF(int distances[])
{
    Serial.print("Distances (mm): ");

    // Do n-valid Readings, 3 Attemps (j) per Reading
    for (int i = 0; i < sampleSize; i++)
    {
        uint8_t j = 0;
        uint16_t distance = 0;
        bool validReading;

        while (!validReading && j < 3)
        {
            // status = toF.rangingTest(&measure, false);
            // distance = measure.RangeMilliMeter;
            distance = toF.readRange();
            // while(toF.waitRangeComplete()) {}

            if (distance > maxDist || distance < 0)
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
Median / 50th Percentile
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

    // inter-measurement period 100ms
    toF.startRangeContinuous(100);

    while (i < sampleSize)
    {
        if(toF.isRangeComplete())
        {
            int distance = toF.readRange();
            // distances.push_back(distance);
            distances[i] = distance;
        }

        if (toF.timeoutOccurred())
        {
            Serial.print("ToF Timeout");
        }

        i++;
    }

    toF.stopRangeContinuous();
}

void Cistern::driveSolenoid(uint8_t relaisChannel, uint8_t state)
{
    digitalWrite(relaisPins[relaisChannel], state);
}