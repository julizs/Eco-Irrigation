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

  Serial.print("SoilMoisture_AnalogVoltage: ");
  Serial.print(moistureSmoothed);
  Serial.println("mV");

  return moistureSmoothed;
}

/*
Measure pin(s) per Plant, calc Avg
Write to Point
*/
void SoilMoisture::measurePlant(JsonArray &moistureSensors, Point &p)
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
    
      int pinNum = moistureSensor; // Measure pinNum on Multiplexer

      char key[16];
      snprintf(key, 16, "soilMoisture_S%d", moistureSensor);
      Serial.println(key);

      int moistureSmoothed = measureSoilMoistureSmoothed(pinNum);
      int moisturePercentage = voltageToPercentage(pinNum, moistureSmoothed);
      avgPerc += moisturePercentage;
      numSensors++;
      p.addField(key, moisturePercentage);
    }

  avgPerc = avgPerc / numSensors;
  // p.addField...
}

/*
Main calls setSensorRanges first (once, not per Plant)
Convert read voltage Value to Percentage (depending on individual Sensor Range)
*/
int SoilMoisture::voltageToPercentage(int pinNum, int moistureSmoothed)
{
  int range[2] = {500,3000}; // default
  getSensorRange(pinNum, range); // pass by ref
  // Serial.println(range[0]);
  
  // Constrain measured Values to sensorRange before mapping
  int moistureConstrained = constrain(moistureSmoothed, range[0], range[1]);

  // Map with reversed Values (3000 = 0% Moisture) to %
  int moisturePercentage = map(moistureConstrained,range[0],range[1], 100, 0);

  Serial.print("SoilMoisture_Percentage: ");
  Serial.print(moisturePercentage);
  Serial.println("%");
  
  return moisturePercentage;
}

/*
Calibration, get individual sensor Ranges
(Array: Call by Reference)
*/
void SoilMoisture::getSensorRange(int pinNum, int range[])
{
  if(sensorRanges.isNull())
    SoilMoisture::setSensorRanges();

  // Serial.println(sensorRanges[pinNum]["minVoltage"].as<int>());
  JsonObject sensorRange = sensorRanges[pinNum].as<JsonObject>();
  
  if(sensorRange.isNull())
  {
    // keep standard values
    Serial.println("No Calibration for moisture Sensor found.");
  }
  else
  { 
    // Serial.println(sensorRange["minVoltage"].as<int>());
    // range[0] = sensorRanges[pinNum]["minVoltage"].as<int>();
    // range[1] = sensorRanges[pinNum]["maxVoltage"].as<int>();
    range[0] = sensorRange["minVoltage"].as<int>();
    range[1] = sensorRange["maxVoltage"].as<int>();
  }
}

// Do only once
void SoilMoisture::setSensorRanges()
{
  Services::doJSONGetRequest("/sensors/soilMoisture", sensorRanges);
  Serial.println("Loaded moisture Sensor Calibration Data.");
}

void SoilMoisture::writePoint(Point &p)
{
  
}