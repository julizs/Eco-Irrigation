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
  /* Analoger Fotoresistor
  https://www.geekering.com/categories/embedded-sytems/esp8266/ricardocarreira/esp8266-nodemcu-simple-ldr-luximeter/
  */
  int rawVal = analogRead(pin);
  float Vout = float(rawVal) * (vin / float(1023));// Conversion analog to voltage
  float RLDR = (res * (vin - Vout))/Vout; // Conversion voltage to resistance
  int lux = 500/(RLDR/1000); // Conversion resitance to lumen

  return rawVal;
}