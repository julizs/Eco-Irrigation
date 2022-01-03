#include "soilmoisture.h"

SoilMoisture::SoilMoisture(int sensorFloor, byte sampleSize, byte multiplexerPin)
{
  this->sensorFloor = sensorFloor;
  this->sampleSize = sampleSize;
  this->multiplexerPin = multiplexerPin;
}

int SoilMoisture::measureSoilMoistureSmoothed()
{
  int sum = 0;

  // Analog Smoothing
  for(int i = 0; i < sampleSize; i++)
  {
    int soilMoisture = measureSoilMoisture();

    if(isnan(soilMoisture))
	  {
		 Serial.println("Soil Moisture Measurement failed.");
	  }

    sum += soilMoisture;
    delay(1);
  }

  int smoothedMoisture = sum/sampleSize;

  return smoothedMoisture;
}

int SoilMoisture::measureSoilMoisture()
{
  // Range: 0-1023, 1023 is maximum Dryness
  
  //int rawValue = analogRead(analogPin);
  int rawValue = Multiplexer::readChannel(multiplexerPin);
  // measured Value sometimes under Floor, constrain needed for calc of percentage
  int constrainedValue = constrain(rawValue, sensorFloor, 1024);

  return rawValue;
}

float SoilMoisture::calcPercentage(int constrainedValue)
{
  int mappedValue = map(constrainedValue,1024,sensorFloor,0,100);
  float percentageValue = ( 100.0f - ( (mappedValue/1023.0f) * 100.0f ) );
  return percentageValue;
}