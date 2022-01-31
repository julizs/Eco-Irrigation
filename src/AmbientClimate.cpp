#include "AmbientClimate.h"

AmbientClimate::AmbientClimate(int maxPollingRate, int measureAttempts)
{
  this->maxPollingRate = maxPollingRate;
  this->measureAttemps = measureAttemps;
}

void AmbientClimate::setup()
{
  dht.setup(DHTpin, DHTesp::DHT11);
}

void AmbientClimate::loop()
{
  switch (currentState)
  {
  case MeasureState::MEASURE:
    if (lastState != currentState) // Do once
    {
      measurementsComplete = false;
      Serial.println("DHT11 measure.");
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

    measurementsComplete = true;
    //return data; // Return valid values immediately

    break;

  case MeasureState::REMEASURE:
    if (lastState != currentState) // Do once
    {
      Serial.println("DHT11 remeasure.");
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
}

bool AmbientClimate::validHumidity()
{
  if (isnan(humidity) || humidity >= dht.getUpperBoundHumidity() || humidity <= dht.getLowerBoundHumidity())
  {
    return false;
  }
  return true;
}

bool AmbientClimate::validTemperature()
{
  if (isnan(temperature) || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature())
  {
    return false;
  }
  return true;
}

float AmbientClimate::measureHumidityDHT()
{
  float humidity = dht.getHumidity();
  return humidity;
}

float AmbientClimate::measureTemperatureDHT()
{
  float temperature = dht.getTemperature();
  return temperature;
}
