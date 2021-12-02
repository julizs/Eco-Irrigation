#include "fotoresistor.h"
#include "pins.h"

Fotoresistor::Fotoresistor(int resistance, float voltageIn, uint8_t pinNum)
{
  res = resistance; 
  vin = voltageIn;
  pin = pinNum;
}

int Fotoresistor::measureLight()
{
  int rawVal = analogRead(pin);

  return measureLux(rawVal);
}

float Fotoresistor::measureLightSmoothed()
{
  // Analog Input with low Polling Rate, very fast consecutive Measurements
  // without delay possible
  float sum = 0.0f;

  for(int i = 0; i < measureAttempts; i++)
  {
    sum += analogRead(pin);
  }

  float rawVal = sum/measureAttempts;
  
  return measureLux(rawVal);
}

int Fotoresistor::measureLux(float rawVal)
{
   float Vout = float(rawVal) * (vin / float(1023));// Conversion analog to voltage
   float RLDR = (res * (vin - Vout))/Vout; // Conversion voltage to resistance
   int lux = 500/(RLDR/1000); // Conversion resitance to lumen

   return lux;
}