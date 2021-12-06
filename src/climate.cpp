#include "climate.h"

Climate::Climate(int maxPollingRate, int measureAttempts)
{
  this->maxPollingRate = maxPollingRate;
  this->measureAttemps = measureAttemps;
}

void Climate::setup()
{
  dht.setup(DHTpin, DHTesp::DHT11);
}

DHTdata Climate::loop()
{
  switch (currentState)
  {
  case MeasureState::MEASURE:
    if (lastState != currentState) // Do once
    {
      Serial.println("DHT11 Measure.");
      data.humidity = dht.getHumidity();
      data.temperature = dht.getTemperature();
      currentState = MeasureState::IDLE;
      stateBeginMillis = millis();
    }
    break;

  case MeasureState::IDLE:

    if (lastState != currentState)
    {
      Serial.println("DHT11 idle.");
    }

    if (!validHumidity() || !validTemperature())
    {
      // Remeasure after maxPollingRate Interval if a value is not valid
      if (millis() - stateBeginMillis > maxPollingRate)
      {
        currentState = MeasureState::REMEASURE;
        lastState = MeasureState::IDLE;
        stateBeginMillis = millis();
      }
    }
    return data; // Return valid values immediately
    break;

  case MeasureState::REMEASURE:
    if (lastState != currentState) // Do once
    {
      // Only remeasure non-valid value
      if (!validHumidity())
      {
        data.humidity = dht.getHumidity();
      }

      if (!validTemperature())
      {
        data.temperature = dht.getTemperature();
      }

      currentState = MeasureState::IDLE;
      lastState = MeasureState::REMEASURE;
      measureAttemps++;
      stateBeginMillis = millis();
    }
    break;
  }

  return data;
}

bool Climate::validHumidity()
{
  if (isnan(humidity) || humidity >= dht.getUpperBoundHumidity() || humidity <= dht.getLowerBoundHumidity())
  {
    return false;
  }
  return true;
}

bool Climate::validTemperature()
{
  if (isnan(temperature) || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature())
  {
    return false;
  }
  return true;
}

float Climate::measureHumidityDHT()
{
  float humidity = dht.getHumidity();
  return humidity;
}

float Climate::measureTemperatureDHT()
{
  float temperature = dht.getTemperature();
  return temperature;
}
