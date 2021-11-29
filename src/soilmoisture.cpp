#include "soilmoisture.h"
#include "Arduino.h"

SoilMoisture::SoilMoisture(int sensorErrorRate)
{
  sensorErrorRate = sensorErrorRate;
}

float SoilMoisture::SmoothedSoilMoisture()
{
  float smoothedMoisture = 0;

  // Analog Smoothing
  for(int i = 0; i < sampleRate; i++)
  {
    float soilMoisture = MeasureSoilMoisture();
    Serial.println(soilMoisture);

    if(isnan(soilMoisture))
	  {
		 Serial.println("Soil Moisture Measurement failed.");
	  }

    smoothedMoisture += soilMoisture; 
  }

  return smoothedMoisture;
}

float SoilMoisture::MeasureSoilMoisture()
{
  // Range: 0-1023, 1023 ist max. Trockenheit
  
  float rawValue = analogRead(analogPin);
  float constrainedValue = constrain(rawValue, sensorErrorRate, 1024); // da manchmal Ausreißer unterhalb "Floor" in Wasser, z.B. 350)
  float mappedValue = map(constrainedValue,1024,sensorErrorRate,0,100);
  //float percentageValue = ( 100.00 - ( (mappedValue/1023.00) * 100.00 ) );

  return rawValue;
}