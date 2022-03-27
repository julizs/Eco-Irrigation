#include <AmbientLight.h>

AmbientLight::AmbientLight(int sensorId)
{
  this->sensorId = sensorId;
  Adafruit_TSL2591 tsl = Adafruit_TSL2591();
}

void AmbientLight::setupTSL2591(TwoWire &bus)
{
  // Only rerun setup if status is 0, see library
  if (tsl.getStatus() == 0)
  {
    if (tsl.begin(&bus)) // &I2Ctwo
    {
      // Sensor Details (Name, Version, ID, min/max, Res)
      sensor_t sensor;
      tsl.getSensor(&sensor);
      Serial.print("Version: ");
      Serial.print(sensor.version);
      Serial.println();

      // Adapt to brighter/dimmer light situations with gain & integrationTime
      //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
      tsl.setGain(TSL2591_GAIN_MED); // 25x gain
      //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain

      // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest (bright light)
      tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS); // "Belichtungszeit"
      // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest (dim light)

      Serial.print("Gain: ");
      Serial.print(tsl.getGain());
      Serial.println();
      Serial.print("Timing: ");
      long timing = (tsl.getTiming() + 1) * 100;
      Serial.print(timing);
      Serial.println();

      Serial.print("did Setup tsl2591 with ID: ");
      Serial.print(sensorId);
      Serial.println();
    }
  }
  else
  {
    Serial.print("could not setup tsl2591 with ID: ");
    Serial.print(sensorId);
    Serial.println();
    // while (1); // Wait infinitely till Sensor initialised
  }
}

TSL2591data AmbientLight::measureLight()
{
  TSL2591data data;

  // Read all 32 bits, top 16 bits IR, bottom 16 bits full spectrum (Bitschiebeop., Bitmaske)
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;

  data.infraRed = ir;
  data.visibleLight = (full - ir);

  Serial.print(F("[ "));
  Serial.print(millis());
  Serial.print(F(" ms ] "));
  Serial.print(F("IR: "));
  Serial.print(ir);
  Serial.print(F("  "));
  Serial.print(F("Full: "));
  Serial.print(full);
  Serial.print(F("  "));
  Serial.print(F("Visible: "));
  Serial.print(full - ir);
  Serial.print(F("  "));
  Serial.print(F("Lux: "));
  Serial.println(tsl.calculateLux(full, ir), 6);

  return data;
}