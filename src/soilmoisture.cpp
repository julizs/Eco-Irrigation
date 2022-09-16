#include "SoilMoisture.h"

DynamicJsonDocument SoilMoisture::sensorRanges(1024);


int SoilMoisture::measureSoilMoisture(int pinNum)
{
  // Moisture Range: around 1000-3000 @ 5V, 3000 is max Dryness
  int moistureRaw = Multiplexer::readChannel(pinNum);
  return moistureRaw;
}

/*
Make multiple Measurements per Sensor, take Avg/Smooth
*/
int SoilMoisture::measureSoilMoistureSmoothed(int pinNum)
{
  int sum = 0;
  int sampleSize = 4;
  int actualSamples = 0;

  // Analog Smoothing
  for(int i = 0; i < sampleSize; i++)
  {
    int moistureRaw = SoilMoisture::measureSoilMoisture(pinNum);

    if(!isnan(moistureRaw))
	  {
		 sum += moistureRaw;
         actualSamples++;
	  }
      delay(10);
  }

  int moistureSmoothed = sum/(actualSamples*1.0f);

  /*
  Serial.print(moistureSmoothed);
  Serial.print(" Measured MoistureSensor: ");
  Serial.print(pinNum + 1);
  Serial.print(" on Multiplexer Channel: ");
  Serial.print(pinNum);
  Serial.print("Valid SoilMoist Samples: ");
  Serial.print(actualSamples);
  Serial.println();
  */

  return moistureSmoothed;
}

/*
Measure pin(s) per Plant, calc Avg
Write to Point
*/
void SoilMoisture::measurePlant(JsonArray moistureSensors)
{
  if (moistureSensors.isNull())
    {
      Serial.println("Plant has no assigned moisture Sensors.");
      return;
    }

  int numSensors = 0;
  float avgPerc = 0.0f;

  for (int i = 0; i < moistureSensors.size(); i++)
    {
      int moistureSensor = moistureSensors[i];
      int pinNum = moistureSensor - 1;

      char key[16]; // InfluxDB Key for Point
      snprintf(key, 16, "soilMoisture%d", moistureSensor);

      int moistureSmoothed = measureSoilMoistureSmoothed(pinNum);
      int moisturePercentage = voltageToPercentage(pinNum, moistureSmoothed);
      avgPerc += moisturePercentage;
      numSensors++;
      // p.addField(key, moisturePercentage);
    }

  avgPerc = avgPerc / numSensors;
  // p.addField...
}

/*
Main calls setSensorRanges first (once, not per Plant)
Convert read voltage Value to Percentage (depending on individual Sensor Range)
*/
int SoilMoisture::voltageToPercentage(int pinNum, int smoothedValue)
{
  int range[2];
  JsonArray sensorRange = sensorRanges[pinNum];

  if(sensorRange.isNull())
  {
    // Set approx. dummy Ranges
    Serial.println("No Ranges for Moisture Sensor found.");
    range[0] = 500;
    range[1] = 3000;
  }
  else
  {
    range[0] = sensorRanges[pinNum]["minVoltage"].as<int>();
    range[1] = sensorRanges[pinNum]["maxVoltage"].as<int>();
  }

  // Constrain measured Values to sensorRange before mapping
  int moistureConstrained = constrain(smoothedValue, range[0], range[1]);

  // Map with reversed Values (3000 = 0% Moisture) to %
  int moisturePercentage = map(moistureConstrained,range[0],range[1], 100, 0);
  
  return moisturePercentage;
}

/*
Get individual sensorRange
(Array: Call by Reference)
*/
void SoilMoisture::getSensorRange(int pinNum, int range[])
{
  if(sensorRanges.isNull())
    SoilMoisture::setSensorRanges();

  range[0] = sensorRanges[pinNum]["minVoltage"].as<int>();
  range[1] = sensorRanges[pinNum]["maxVoltage"].as<int>();
}

void SoilMoisture::setSensorRanges()
{
  Services::doJSONGetRequest("/moistureSensors", sensorRanges);
}