#include "SoilMoisture.h"

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

  Serial.print(moistureSmoothed);
  Serial.print(" Measured MoistureSensor: ");
  Serial.print(pinNum + 1);
  Serial.print(" on Multiplexer Channel: ");
  Serial.print(pinNum);
  // Serial.print("Valid SoilMoist Samples: ");
  // Serial.print(actualSamples);
  Serial.println();

  return moistureSmoothed;
}

int SoilMoisture::measureSoilMoisture(int pinNum)
{
  // Moisture Range: around 1000-3000 @ 5V, 3000 is max Dryness
  int moistureRaw = Multiplexer::readChannel(pinNum);
  return moistureRaw;
}

int SoilMoisture::voltageToPercentage(int pinNum, int smoothedValue)
{
  // Constrain measured Values to SensorRange before Map
  int range[2];
  getSensorRange(pinNum, range);
  int moistureContrained = constrain(smoothedValue, range[0], range[1]);

  // Serial.print("Min: "); Serial.print(range[0]);
  // Serial.print(" Max: "); Serial.println(range[1]);

  int moisturePercentage = -1;
  if(range[0] != 0 && range[1] != 0)
  {
    // Map with reversed Values (3000 = 0% Moisture)
    moisturePercentage = map(moistureContrained,range[0],range[1], 100, 0);
  }
  
  return moisturePercentage;
}

void SoilMoisture::getSensorRange(int pinNum, int range[])
{
  // Send Request to REST API and get min and max Voltage, which varies
  // from Sensor to Sensor (pinNum is Sensor ID, from 0 - 15)
  // C++ passes Arrays to Funcs with Call by Reference, Original Array gets changed

  char url[] = "https://juli.uber.space/node/moistureSensors";
  DynamicJsonDocument doc = services.doJSONGetRequest(url);
  range[0] = doc[pinNum]["minVoltage"].as<int>();
  range[1] = doc[pinNum]["maxVoltage"].as<int>();
}