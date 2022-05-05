#include <AmbientLight.h>

AmbientLight::AmbientLight(int sensorId)
{
  // 1 or 2nd Sensor (if 2 Sensors at once worked)
  this->sensorId = sensorId;
  Adafruit_TSL2591 tsl = Adafruit_TSL2591();
}

void AmbientLight::setupTSL2591(TwoWire &bus)
{
  // Only rerun setup if status is 0 (not initialized), see library
  if (!isReady())
  {
    if (tsl.begin(&bus)) // &I2Ctwo
    {
      // Adapt to brighter/dimmer light situations (Gain & "Belichtungszeit")
      tsl.setGain(TSL2591_GAIN_MED); // 25x gain
      tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS); // medium

      Serial.print("Did Setup tsl2591 with ID: ");
      Serial.println(sensorId);

      printSensorInfo();
    }
    else
    {
      Serial.print("Could not setup tsl2591 with ID: ");
      Serial.println(sensorId);
    }
  }
}

bool AmbientLight::isReady()
{
  // Try to Measure to change Bits
  uint32_t lum = tsl.getLuminosity(1);
  bool ready = tsl.getStatus() != 0;
  if (ready)
    Serial.println("TSL 2591 is ready.");
  return ready;
}

/*
Print Sensor Details (Name, Version, ID, min/max, Res)
*/
void AmbientLight::printSensorInfo()
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.print("Version: ");
  Serial.print(sensor.version);
  Serial.println();

  Serial.print("Gain: ");
  Serial.print(tsl.getGain());
  Serial.println();
  Serial.print("Timing: ");
  long timing = (tsl.getTiming() + 1) * 100;
  Serial.println(timing);
}

TSL2591data AmbientLight::measureLight()
{
  TSL2591data data;

  // Read all 32 bits, top 16 bits IR, bottom 16 bits full spectrum ("Bitschiebeop., Bitmaske")
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