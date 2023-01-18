#include "Cistern.h"

Cistern::Cistern(uint8_t toF_address)
{
    this->toF_address = toF_address;

    // solenoidValves = relaisChannels;

    contents = LiquidType::WATER;

    minValidLiquidLevel = 40;
    // maxValidLiquidLevel = 150;
    maxMeasurableDist = 190; // sensor Height above Cistern Bottom
    currLiquidLevel = 0;
    currLiquidAmount = 0;
    sampleSize = 8;
}

/*
Wrong Readings if spadCount after (re)setup is different (e.g. 3 instead of 4 spads)
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

    Serial.print("ToF Test: ");
    Serial.println(measurement.RangeMilliMeter);
    Serial.print("Error: ");
    Serial.println(toF.Status);

    return toF.Status == 0;
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

/*
maxPossibleDist = cisternHeight - (cisternHeight - sensorHeight) - waterBodyMinHeight
e.g. 210mm - (210mm -190mm) - 20mm
= 210mm -20mm -20mm = 170mm

currWaterevel = waterBodyMinHeight + maxPossibleDist - currWaterDist
e.g. 170mm - 170mm  + 20mm = 20mm (min. erfassbar)
e.g. 170mm - 135mm + 20mm= 55mm

Use max func to prevent negative delta
170mm -171mm = -1mm
max(170-171),0 = max(-1,0) = 0
*/
uint16_t Cistern::getLiquidLevel()
{ 
    int minMeasurableDistance = 15; // mm, minHeight of swimmer Object
    int sensorError = -15; // mm
    currLiquidDist = evaluateToF();
    currLiquidLevel = max((maxMeasurableDist - (currLiquidDist + sensorError)),0);
    // currLiquidLevel = minMeasurableDistance + max((maxMeasurableDist - (currLiquidDist + sensorError)),0); 
    
    return currLiquidLevel;
}

uint16_t Cistern::getLiquidAmount()
{ 
    currLiquidAmount = calcMl(getLiquidLevel());

    Serial.print("Available Water (Ml): ");
    Serial.println(currLiquidAmount);

    return currLiquidAmount;
}

/*
Assert if waterLevel is valid/sufficient, by comparing available
liquidVolume in Cistern and allocatedLiquid of an Irrigation Event (automatic or manual)
(same procedure as validSolenoid)
*/
bool Cistern::validLiquidLevel(int allocatedLiquid = 0)
{
    bool isValid;

    currLiquidAmount = getLiquidAmount();
    uint16_t minValidLiquidVolume = calcMl(minValidLiquidLevel);

    if(allocatedLiquid > 0)
    {
        isValid = currLiquidAmount - allocatedLiquid >= minValidLiquidVolume ? true : false;
    }
    else
    {
        isValid = currLiquidAmount > minValidLiquidVolume;
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
    int oldLiquidAmount = currLiquidAmount; // Calculated and submitted by PUMP::INIT
    currLiquidAmount = getLiquidAmount();

    // prevent undefined values for InfluxDB (measureError or Aborted)
    uint16_t pumpedLiquid = max((oldLiquidAmount - currLiquidAmount),0);

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
    p0.addField(key1, currLiquidLevel);
    snprintf(key2, 32, "%s_amount", "water");
    p0.addField(key2, currLiquidAmount);

    // Serial.println(p0.toLineProtocol());
    InfluxHelper::writeDataPoint(p0);
}

/*
Measurements:
(9 Liters = 102mm)
(8 Liters = 110mm)
(7 Liters = 118mm)
(6 Liters = 127mm) (90mm from Boxbottom)
5 Liters = 136mm from Sensor
4 Liters = 146mm
3 Liters = 156mm
2 Liters = 167mm (minValid)
0,1 Liter: = 179mm (Unsafe for Pump)
Min (Safe): 2 Liters (-> < 167mm), Max(Safe): 9 Liters (> 102mm)
Rule:
Rounded: 10mm/1cm = 1L -> 1mm = 100ml
Precise: -1mm every 2 Liters higher (Cistern gets wider) -> 100ml more every 2L/2cm higher (?)

2 Formeln:
Näherungsverfahren (ohne Integral) Keplersche Fassregel 
(nicht nötig für Frustum, da Spezialfall eines Prismatoids mit einfacher Berechnung):
ODER
Volumen of Frustum, Problem 14:
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

Examples:
Full Reservoir (h=0.21):
sqrt(0,005464) = 0.0739
h/3 * 0,2222 = 0,0155
0,0155 * 1000 = 15 Liter

Fill Level 90mm (h = 0.09):
0.09m * 0.15m = 0.0135m (Slope, = 1.35cm for 9cm)
l_T = 0.29 + 0.0135 = 0.3035m 
w_T = 0.22 + 0.0135 = 0.2335m
A_B = 0.0638m² (Bottom)
A_T = 0.07086725m² (Top)

waterVolume = 0.09/3 * (0.0638 + 0.07086725 + sqrt(0.0638 * 0.07086725))
= 0.03 * (0.13466725 + sqrt(0.00452133055))
= 0.03 * (0.13466725 + 0,0672408398965986)
= 0.03 * 0.2019080898966
= 0.00605724269m³ = 6.057 Liter (Nachgemessen: 9cm, 6 Liter, Korrekt)
(Ohne Slope Berücksichtigung: 0.0638 * 0.09 = 0.005742 m³ = 5.74 Liter)
*/
uint16_t Cistern::calcMl(int waterLevel) // Inputparam in mm
{
    float l_B = 290, w_B = 220; // m
    float l_T = 320, w_T = 250; // -> 3cm Slope auf 20cm, 1.5cm auf 10cm
    float A_B = l_B * w_B; // Area Bottom (doesn't change), 0,0638m², 1m³ = 1000L
    // double A_T = l_T * w_T; // Area Top (changes depending on fillLevel), 0.08m²
    // int h = 210;

    // Measure slope length, slope width of Reservoir
    // (1.5cm per 10cm, 15cm (0.15m) per 1m, 0.15cm (1.5mm) per 1cm, 0.15mm per 1mm)
    float sl_l = 0.15, sl_w = 0.15; // mm je mm
    
    // 1. Set Height
    // Min: h = 0mm
    uint16_t h = waterLevel;

    // 2. Slope in mm per m (for a given waterLevel):
    // e.g. 33mm * 0.15mm/mm = 4.95mm
    float sl_l_eff = waterLevel * sl_l;
    float sl_w_eff = waterLevel * sl_w;

    // 3. Calc A_T (new top area):
    // (Seitenlänge unten + effective Slope * 2 für gegebenen Füllstand)
    // 294.95mm * 224.95mm = 66349mm²)
    // (if l_B, w_B are ints then 294 * 224 = 65856, WRONG)
    // uint32_t nötig, da > 65000ml möglich
    // *2, da slope in beide Richtungen
    l_T = l_B + sl_l_eff * 2;
    w_T = w_B + sl_w_eff * 2;
    uint32_t A_T = l_T * w_T;
    
    // 3. Volumen berechnen (ähnlich auch für Kegel Frustum)
    uint32_t waterVolume = h/3.0f * (A_B + A_T + sqrt(A_B*A_T));

    // uint8_t waterAmount = waterVolume * 1000; // m³ * 1000 = Liter
    uint32_t waterAmount = waterVolume / 1000.0f; // mm³ /1000000 = L, mm³ /1000 = Ml

    return waterAmount; // Outputparam in ml (>250ml possible -> uint16_t)
}

/*
Detailed Data of Pump Process (Affected Plants, PumpTime, Success, ... )
done by Irrigation::reportInstructions 
(or Irrigation::reportToMongo)
*/
/*
void Cistern::updateIrrigationData(uint8_t relaisChannel, int pumpedWater)
{
    //int pumpedWaterML = rand() % 200 + 100; // Demo, 100-300ml

    // Reuse Datapoint, create new Row in InfluxDB Irrigations Measurement
    p2.clearTags();
    p2.clearFields();
    char solenoidValve[4];
    itoa(relaisChannel, solenoidValve, 10);
    p2.addTag("solenoidValve", solenoidValve);
    p2.addField("pumpedWater", pumpedWater);

    InfluxHelper::writeDataPoint(p2);
}
*/

/*
Only way to prevent Crash if one or both ToF setup fail:
Do Test Reading and only then compare err Code
*/
void Cistern::readToF(int distances[])
{
    Serial.print("Distances (mm): ");

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

            if (distance > maxMeasurableDist || distance <= 0)
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

/*
relaisChannel -1, since WebInterface starts counting from 1 not 0
*/
void Cistern::driveSolenoid(uint8_t relaisChannel, uint8_t state)
{
    digitalWrite(relaisPins[relaisChannel], state);
}