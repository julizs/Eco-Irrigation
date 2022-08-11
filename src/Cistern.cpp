#include "Cistern.h"

Cistern::Cistern(uint8_t toF_address, uint8_t relaisChannels[])
{
    this->toF_address = toF_address;

    solenoidValves = relaisChannels;
    pumpedWater = 0;
    minValidWaterDist = 50;
    maxValidWaterDist = 170; // 3L
    maxPossibleDist = 200;
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
    return currWaterDist < maxValidWaterDist && currWaterDist > minValidWaterDist;
}

/*
Sensor is mounted lower than Cistern Height
Moving Water-Body is mounted higher than Cistern Bottom
-> maxPossibleDist = cisternHeight - sensorHeight - waterBodyMinHeight
e.g. 210mm - (210mm -190mm) - 20mm
= 210mm -20mm -20mm
=  170mm
(Diesen Wert durch Messen verifizieren, 
ab hier (z.B. 2L) ist Füllstand messbar, Änderungen darunter nicht erfassbar)

fillLevel = waterBodyMinHeight + maxPossibleDist - currWaterDist
e.g. 20mm + 170mm - 170mm = 0mm
e.g. 20mm + 170mm - 135mm = 55mm

Ergänzung (falls höherer Wert als maxPossibleDist gemessen wird):
fillLevel = min((maxPossibleDist - currWaterDist),0)
170mm -171mm = -1mm
max(170-171),0 = max(-1,0) = 0
*/
int Cistern::updateWaterLevel()
{
    if(toF_ready) // or Crash if called from PumpState::ABORTED and toF not setup
    {
        int waterBodyMinHeight = 10;
        int sensorError = 15; // Sensor measures wrong...
        currWaterDist = evaluateToF();
        currWaterLevel = waterBodyMinHeight + sensorError + max((maxPossibleDist - currWaterDist),0);
    }
    
    return currWaterLevel;
}

/*
Water Management (after Pump Procedure)
Update Waterlevel, write waterLevel and Irrigation Point, give Warnings (low waterLevel etc.)
*/
bool Cistern::waterManagement(uint8_t relaisChannel)
{
    int oldWaterLevel = currWaterLevel; // (Updated from Check at Beginning of Pump State Machine)
    int newWaterLevel = updateWaterLevel(); // Get new fill Level and Update
    uint16_t availableWater = calcMl(newWaterLevel);
    // 20mm pumped can be different waterAmounts, depending on waterLevel
    // min necessary, if pumpProcess stops after 0s AND wrong Reading of new waterLevel
    pumpedWater = max((calcMl(oldWaterLevel) - availableWater),0);
    
    Serial.println("Available Water and pumped Water: ");
    Serial.println(availableWater);
    Serial.println(pumpedWater);

    // Only create new Point if evaluateTof returns valid, else don't create new Grafana Point
    if (newWaterLevel <= oldWaterLevel)
    {
        updateEnvironmentData(newWaterLevel, availableWater);

        // Done by Irrigation::reportInstructions instead, Main class sets Info in Irrigation Instruction Obj
        // updateIrrigationData(relaisChannel, pumpedWater);

        return true; // Created Points, wrote into Buffer
    }

    return false;
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

Volumen of Frustum, Problem 14 (*):
https://en.wikipedia.org/wiki/Moscow_Mathematical_Papyrus

Näherungsverfahren (ohne Integral) Keplersche Fassregel (nicht nötig für Frustum, da Spezialfall eines Prismatoids mit einfacher Berechnung*):
Prismoid calculate Volume / Volume of a Trapezoid tank
Kein Prisma sondern Prismatoid, da Grund- und Deckfläche nicht kongruent
Oder (Pyramid) Frustum? (Ähnl.: Kegel Frustum)
https://www.onlineconversion.com/object_volume_trapezoid.htm
https://www.agric.gov.ab.ca/app19/calc/volume/dugout.jsp
https://www.lernhelfer.de/schuelerlexikon/mathematik/artikel/prismatoid#
https://www.trelleborg.com/apps/avc/index_de.html
http://www.solving-math-problems.com/volume-of-frustum-of-a-pyramid.html
https://www.calculatorsoup.com/calculators/geometry-solids/conicalfrustum.php
https://mathepedia.de/Keplersche_Fassregel.html
https://mathworld.wolfram.com/Prismatoid.html
https://mathworld.wolfram.com/PyramidalFrustum.html (*)
https://www.tagblatt.de/Nachrichten/Das-Fass-und-seine-Formel-400021.html

Examples:
Full Reservoir (h=0.21):
sqrt(0,005464) = 0.0739
h/3 * 0,2222 = 0,0155
0,0155 * 1000 = 15 Liter

Fill Level 90mm (h = 0.09):
0.09m * 0.15m = 0.0135m (Slope, = 1.35cm for 9cm)
l_T = 0.29 + 0.0135 = 0.3035m 
w_T = 0.22 + 0.0135 = 0.2335m
A_T = 0.07086725m²
(A_B = 0.0638m², bleibt gleich)
waterVolume = 0.09/3 * (0.0638 + 0.07086725 + sqrt(0.0638 * 0.07086725))
= 0.03 * (0.13466725 + sqrt(0.00452133055))
= 0.03 * (0.13466725 + 0,0672408398965986)
= 0.03 * 0.2019080898966
= 0.00605724269m³ = 6.057 Liter (Nachgemessen: 9cm, 6 Liter, Korrekt)
(Dumm / Ohne Slope Berücksichtigung: 0.0638 * 0.09 = 0.005742 m³ = 5.74 Liter)
*/
uint16_t Cistern::calcMl(int waterLevel) // Eingabeparam in mm, Ausgabeparam in ml (>250ml möglich)
{
    float l_B = 290, w_B = 220; // m
    float l_T = 320, w_T = 250; // -> 3cm Slope auf 20cm, 1.5cm auf 10cm
    float A_B = l_B * w_B; // Area Bottom (doesn't change), 0,0638m², 1m³ = 1000L
    // double A_T = l_T * w_T; // Area Top (changes depending on fillLevel), 0.08m²
    // int h = 210;
    // slope length, slope width (1.5cm per 10cm, 15cm (0.15m) per 1m, 0.15cm (1.5mm) per 1cm, 0.15mm per 1mm)
    float sl_l = 0.15, sl_w = 0.15; // mm je mm
    
    // 1. Set Height
    // Min: h = 0mm
    uint16_t h = waterLevel;

    // 2. Slope in mm per m (for a given waterLevel):
    // e.g. 33mm * 0.15mm/mm = 4.95mm
    float sl_l_eff = waterLevel * sl_l;
    float sl_w_eff = waterLevel * sl_w;
    Serial.println("Slope:");
    Serial.print(sl_l_eff);
    Serial.println(" mm");

    // 3. Calc A_T (new top area):
    // (Untere Seiten + effective Slope for fillLevel)
    // 294.95mm * 224.95mm = 66349mm²)
    // (if l_B, w_B are ints then 294 * 224 = 65856, WRONG)
    // uint32_t nötig, da > 65000ml möglich
    l_T = l_B + sl_l_eff;
    w_T = w_B + sl_w_eff;
    uint32_t A_T = l_T * w_T; 
    Serial.println("Area A_T:");
    Serial.print(A_T);
    Serial.println(" mm²");
    
    // 3. Volumen berechnen (ähnlich auch für Kegel Frustum)
    uint32_t waterVolume = h/3.0f * (A_B + A_T + sqrt(A_B*A_T));
    Serial.println("Volume: ");
    Serial.print(waterVolume);
    Serial.println(" mm³");

    // uint8_t waterAmount = waterVolume * 1000; // m³ * 1000 = Liter
    uint32_t waterAmount = waterVolume / 1000.0f; // mm³ /1000000 = L, mm³ /1000 = Ml
    Serial.println("Amount: ");
    Serial.print(waterAmount);
    Serial.println(" ml");

    return waterAmount;
}

void Cistern::updateEnvironmentData(int newWaterLevel, int availableWater)
{
    char key1[32], key2[32];
    snprintf(key1, 32, "waterLevel0x%x", this->toF_address);
    p0.addField(key1, newWaterLevel); // Or calc directly in Grafana
    snprintf(key2, 32, "waterAmount0x%x", this->toF_address);
    p0.addField(key2, availableWater);

    // Write Point p0 (Ina and ToF Data) to Buffer
    InfluxHelper::writeDataPoint(p0);
}

/*
Detailed Data of Pump Process (Affected Plants, PumpTime, Success, ... )
done by Irrigation::reportInstructions 
(or Irrigation::reportToMongo)
*/
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

            if (distance > maxPossibleDist || distance <= 0)
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