#include "soilmoisture.h"
#include "pins.h"

SoilMoisture::SoilMoisture(int sensorFloor, byte sampleSize, byte multiPin)
{
  this->sensorFloor = sensorFloor;
  this->sampleSize = sampleSize;
  this->multiPin = multiPin;
}

float SoilMoisture::measureSoilMoistureSmoothed()
{
  float sum = 0;

  // Analog Smoothing
  for(int i = 0; i < sampleSize; i++)
  {
    float soilMoisture = measureSoilMoisture();

    if(isnan(soilMoisture))
	  {
		 Serial.println("Soil Moisture Measurement failed.");
	  }

    sum += soilMoisture; 
  }

  float smoothedMoisture = sum/sampleSize;

  return smoothedMoisture;
}

float SoilMoisture::measureSoilMoisture()
{
  // Range: 0-1023, 1023 ist max. Trockenheit
  
  // Ersetzen durch Multiplexer Aufruf
  float rawValue = analogRead(analogPin);

  float constrainedValue = constrain(rawValue, sensorFloor, 1024); // da manchmal Ausreißer unterhalb "Floor" in Wasser, z.B. 350)
  float mappedValue = map(constrainedValue,1024,sensorFloor,0,100);
  //float percentageValue = ( 100.00 - ( (mappedValue/1023.00) * 100.00 ) );

  return rawValue;
}