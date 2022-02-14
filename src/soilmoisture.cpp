#include "SoilMoisture.h"

int SoilMoisture::measureSoilMoistureSmoothed(int pinNum)
{
  int sum = 0;
  int sampleSize = 4;
  int actualSamples = 0;

  // Analog Smoothing
  for(int i = 0; i < sampleSize; i++)
  {
    int soilMoisture = SoilMoisture::measureSoilMoisture(pinNum);

    if(!isnan(soilMoisture))
	  {
		 sum += soilMoisture;
         actualSamples++;
	  }
      delay(10);
  }

  int smoothedMoisture = sum/(actualSamples*1.0f);

  Serial.print(smoothedMoisture);
  Serial.print(" measured Multiplexer Channel: ");
  Serial.print(pinNum);
  Serial.print(" which is MoistureSensor: ");
  Serial.print(pinNum + 1);
  // Serial.print("Valid SoilMoist Samples: ");
  // Serial.print(actualSamples);
  Serial.println();

  return smoothedMoisture;
}

int SoilMoisture::measureSoilMoisture(int pinNum)
{
  // Value Range: 0-1023 @Esp8266, 1023 is max Dryness
  // Value Range: 1000-3000 @Esp32, 3000 is max Dryness
  // Constrain needed for mappedValue, measurement sometimes under typical individual Floor Level of Sensor
  
  int sensorFloor = 1000;
  int rawValue = Multiplexer::readChannel(pinNum);
  int constrainedValue = constrain(rawValue, sensorFloor, 1024);

  return rawValue;
}

float SoilMoisture::voltageToPercentage(int constrainedValue)
{
  int sensorFloor = 1000;
  int mappedValue = map(constrainedValue,1024,sensorFloor,0,100);
  float percentageValue = ( 100.0f - ( (mappedValue/1023.0f) * 100.0f ) );

  return percentageValue;
}