#include <AmbientLight.h>

AmbientLight::AmbientLight(int sensorId)
{
  // 1 or 2nd Sensor (if 2 Sensors at once worked)
  this->sensorId = sensorId;
  Adafruit_TSL2591 tsl = Adafruit_TSL2591();

  data.fullSpectrum = 0;
  data.infraRed = 0;
  data.visibleLight = 0;
  data.illuminance = 0.0f;
}

void AmbientLight::setup(TwoWire &bus)
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

      printParameters();
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
void AmbientLight::printParameters()
{
  char message[64];
  sensor_t sensor;
  tsl.getSensor(&sensor);
  long timing = (tsl.getTiming() + 1) * 100;

  snprintf(message, 64, "LightMeter: sensorVersion: %d, gain: %d, timing: %.2f", 
  sensor.version, tsl.getGain(), data.illuminance, timing);

  Serial.println(message);
}

LightData AmbientLight::measure()
{
  LightData data;

  // Read all 32 bits, top 16 bits IR, bottom 16 bits full spectrum ("Bitschiebeop., Bitmaske")
  uint32_t luminosity = tsl.getFullLuminosity();

  data.infraRed = luminosity >> 16;
  data.fullSpectrum = luminosity & 0xFFFF;
  data.visibleLight = (data.fullSpectrum - data.infraRed);
  data.illuminance = tsl.calculateLux(data.fullSpectrum, data.infraRed);

  return data;
}

void AmbientLight::printMeasurement(LightData &measurement)
{
  char message[64];
  snprintf(message, 64, "LightMeter: fullSpectrum: %d, infraRed: %d, illuminance: %.2f", 
  data.fullSpectrum, data.infraRed, data.illuminance);

  Serial.println(message);
}