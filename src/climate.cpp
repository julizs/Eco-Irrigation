#include "climate.h"

Climate::Climate(int maxPollingRate, int measureAttempts)
{
  this->maxPollingRate = maxPollingRate;
  this->measureAttemps = measureAttemps;
}

void Climate::InitialiseDHT()
{
  dht.setup(DHTpin, DHTesp::DHT11);
}

DHTdata Climate::MeasureDHT()
{
  DHTdata data;
  int attempts = 0;
  
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  // Gets repeatedly called from measureState (500ms, for ... seconds)
  // Remeasure specific value, only if its faulty
  // Non-blocking (without delay), Buttons etc. can be read in
  if(isnan(humidity) || humidity >= dht.getUpperBoundHumidity()|| humidity <= dht.getLowerBoundHumidity())
  {
    humidity = dht.getHumidity();
  }
  
  if(isnan(temperature) || temperature >= dht.getUpperBoundTemperature() || temperature <= dht.getLowerBoundTemperature())
  {
    temperature = dht.getTemperature();
  }

  data.humidity = humidity;
  data.temperature = temperature;

  /*
  // Measure once per 120 sec, correct Value crucial
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

float Climate::MeasureHumidityDHT()
{
  float humidity = dht.getHumidity();
  return humidity;
}

float Climate::MeasureTemperatureDHT()
{
  float temperature = dht.getTemperature();
  return temperature;
}
