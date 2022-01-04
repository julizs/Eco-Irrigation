#include <ambientLight.h>

AmbientLight::AmbientLight(int sensorId, TwoWire &bus) : bus(bus) 
{
  this->sensorId = sensorId;
  Adafruit_TSL2591 tsl = Adafruit_TSL2591();
}

void AmbientLight::setupTSL2591()
{
  if (tsl.begin(&I2Ctwo)) // &bus geht nicht
  {
    Serial.println("did Setup tsl2591.");
  } 
  else 
  {
    Serial.println(F("tsl2591 not found."));
    // while (1);
  }

  // Sensor Details (Name, Version, ID, min/max, Res)
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println(sensor.version);
  
  // Adapt to brighter/dimmer light situations with gain & integrationTime

  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest (bright light)
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS); // "Belichtungszeit"
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest (dim light)

  Serial.println(tsl.getGain());
  Serial.println(((tsl.getTiming() + 1) * 100, DEC) +  "ms");
}


void AmbientLight::readTSL2591()
{
  // Read all 32 bits, top 16 bits IR, bottom 16 bits full spectrum (Bitschiebeop., Bitmaske)
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;

  // Calc of Lux? Docu
  Serial.print(F("[ ")); Serial.print(millis()); Serial.print(F(" ms ] "));
  Serial.print(F("IR: ")); Serial.print(ir);  Serial.print(F("  "));
  Serial.print(F("Full: ")); Serial.print(full); Serial.print(F("  "));
  Serial.print(F("Visible: ")); Serial.print(full - ir); Serial.print(F("  "));
  Serial.print(F("Lux: ")); Serial.println(tsl.calculateLux(full, ir), 6);
}