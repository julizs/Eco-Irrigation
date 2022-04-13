#include "Utilities.h"

void Utilities::scanI2CBus(TwoWire *wire)
{
  Serial.println("Scanning I2C Addresses Channel: ");
  uint8_t cnt = 0;
  for (uint8_t i = 0; i < 128; i++)
  {
    wire->beginTransmission(i);
    uint8_t ec = wire->endTransmission(true);
    if (ec == 0)
    {
      if (i < 16)
        Serial.print('0');
      Serial.print(i, HEX);
      cnt++;
    }
    else
      Serial.print("..");
    Serial.print(' ');
    if ((i & 0x0f) == 0x0f)
      Serial.println();
  }
  Serial.print("Scan Completed, ");
  Serial.print(cnt);
  Serial.println(" I2C Devices found.");
}

/*
  https://arduinojson.org/v6/api/staticjsondocument/
  https://arduinojson.org/v6/how-to/store-a-json-document-in-eeprom/
  https://arduinojson.org/v6/how-to/determine-the-capacity-of-the-jsondocument/
  https://arduinojson.org/v6/assistant/
  EEPROM library on the ESP32 allows using at most 1 sector (4kB, 4096 Bytes) of Flash
  Copy Paste Json into ArduinoJson Assistant to see recommended Size (Bytes)
  StaticJsonDocument<1024> doc
  DynamicJsonDocument(2048);
  */
void Utilities::writeDoc(int address, DynamicJsonDocument &doc, int size)
{
  EepromStream eepromStream(address, size);
  serializeJson(doc, eepromStream);
  EEPROM.commit();
}

DynamicJsonDocument Utilities::readDoc(int address, int size)
{
  DynamicJsonDocument doc(size);
  EepromStream eepromStream(address, size);
  deserializeJson(doc, eepromStream);

  return doc;
}

/*
MongoDB Request: assignable pump Models and voltageRanges of moistureSensors
3 Attempts per Request
InfluxDB Request: Query and Writing take long, do sparsely / use EEPROM
(So waterLevel Evaluation on Button Press is fast)
*/
/*
bool Utilities::provideData()
{
  moistureSensors = Services::doJSONGetRequest("/moistureSensors");
  plants = Services::doJSONGetRequest("/plants/sensors");
  plantNeeds = Services::doJSONGetRequest("/plants/needs");
  pumps = Services::doJSONGetRequest("/pumps/json");

  FluxQueryResult cursor = Irrigation::recentIrrigations(2);
  bool wroteCursor = Irrigation::writeVector(cursor);

  Serial.println(moistureSensors.isNull());
  Serial.println(plants.isNull());
  Serial.println(plantNeeds.isNull());
  Serial.println(pumps.isNull());
  Serial.println(wroteCursor);

  if(!moistureSensors.isNull() && !plants.isNull() && !pumps.isNull() && wroteCursor)
    return true;
  return false;
}
*/