#include "climate.h"

Climate::Climate(int number)
{
  testNumber = number;
}

void Climate::InitialiseDHT()
{
  dht.setup(DHTpin, DHTesp::DHT11);
}

DHTdata Climate::MeasureDHT()
{
  DHTdata data;

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("DHT Measurement failed.");
  }

  data.humidity = humidity;
  data.temperature = temperature;

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
