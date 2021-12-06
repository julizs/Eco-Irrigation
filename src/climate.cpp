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
  DHTdata data;
  int attempts = 0;

  switch (currentState)
  {
  case MeasureState::IDLE:

    if (lastState != currentState) // Do once
    {
      Serial.println("DHT11 idle.");
    }

    // Remeasure after maxPollingTime only missing value, non-blocking (no delay)
    if (millis() - maxPollingRate > stateBeginMillis)
    {
      if (isnan(humidity) || humidity >= dht.getUpperBoundHumidity() || humidity <= dht.getLowerBoundHumidity() || 
      isnan(temperature) || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature())
      {
        stateBeginMillis = millis(); // Must only be executed once, e.g. right before state transfer
        currentState = MeasureState::MEASURING;
      }
    }
    lastState = MeasureState::IDLE;
    break;

  case MeasureState::MEASURING:
    if (lastState != currentState) // Do once
    {
      data.humidity = dht.getHumidity();
      data.temperature = dht.getTemperature();
      stateBeginMillis = millis();
      currentState = MeasureState::IDLE;
      lastState = MeasureState::MEASURING;
    }
    break;
  }

  /*
  // Do multiple Measurements since its only every 5 Minutes
  // "inofficial" pollingRate of DHT = 500ms
  // while Loops and delays are blocking Thread
  while (attempts <= 2 &&(isnan(temperature) || isnan(humidity) || humidity >= dht.getUpperBoundHumidity()|| humidity <= dht.getLowerBoundHumidity()
  || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature()))
  {
    Serial.println("DHT Measurement failed.");
    delay(maxPollingRate);
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    attempts++;
  }
  */

  return data;
}

/* DHTdata Climate::measureClimateDHT()
{

} */

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
