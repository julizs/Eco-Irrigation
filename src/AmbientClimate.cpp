#include "AmbientClimate.h"

AmbientClimate::AmbientClimate(float maxPollingRate, uint8_t pinNum)
{
  this->maxPollingRate = maxPollingRate;
  this->pinNum = pinNum;
  minStateTime = 1.0f;
}

void AmbientClimate::setup()
{
  dht.setup(34, DHTesp::AM2302);
  Serial.println("DHT22 Setup called.");
}

void AmbientClimate::loop()
{
  switch (currentState)
  {
  case DhtState::IDLE:

    if (lastState != currentState)
    {
      stateBeginMillis = millis();
      Serial.println("DHT22 Idle.");

      sensorInfo();
    }

    if (millis() - stateBeginMillis > minStateTime)
    {
      currentState = DhtState::MEASURE;
    }

    lastState = DhtState::IDLE;

    break;

  case DhtState::MEASURE:

    // Entry Func, do once
    if (lastState != currentState)
    {
      stateBeginMillis = millis();
      Serial.println("DHT22 Measure.");

      data.humidity = dht.getHumidity();
      data.temperature = dht.getTemperature();
    }

    // Check after PollingRate (2s for DHT22) (or state minTime)
    if (millis() - stateBeginMillis > maxPollingRate)
    {
      // Stay in State and recheck every 2s, or transition to Self to remeasure
      if (!validHumidity(data.humidity) || !validTemperature(data.temperature))
      {
        lastState = DhtState::IDLE; // to run Entry Func again
        currentState = DhtState::MEASURE;
      }
      else
      {
        lastState = DhtState::MEASURE;
        currentState = DhtState::DONE;
      }
    }

    break;

  case DhtState::DONE:

    if (lastState != currentState)
    {
      stateBeginMillis = millis();
      writePoint();
    }

    lastState = DhtState::DONE;
    break;
  }
}

void AmbientClimate::sensorInfo()
{
  char message[128];
  snprintf(message, 128, "Climate Sensor: Model: %d, maxPolling/minSample: %dms, Status: %d",
           dht.getModel(), dht.getMinimumSamplingPeriod(), dht.getStatus());
  Serial.println(message);

  /*
  Serial.println(dht.getModel());
  Serial.println(dht.getMinimumSamplingPeriod());
  Serial.println(dht.getStatus());
  */
}

void AmbientClimate::writePoint()
{
  p0.addField("ambientTemperature", data.temperature);
  p0.addField("ambientHumidity", data.humidity);
}

float AmbientClimate::measureHumidity()
{
  float humidity = dht.getHumidity();
  return humidity;
}

float AmbientClimate::measureTemperature()
{
  float temperature = dht.getTemperature();
  return temperature;
}

bool AmbientClimate::validHumidity(float humidity)
{
  if (isnan(humidity) || humidity >= dht.getUpperBoundHumidity() || humidity <= dht.getLowerBoundHumidity())
  {
    return false;
  }
  return true;
}

bool AmbientClimate::validTemperature(float temperature)
{
  if (isnan(temperature) || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature())
  {
    return false;
  }
  return true;
}
