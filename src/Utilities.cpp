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

void Utilities::provideData()
{
  // Write pump Models and voltageRanges of moistureSensors to EEPROM
  DynamicJsonDocument moistureSensors = Services::doJSONGetRequest("/moistureSensors");
  DynamicJsonDocument pumps = Services::doJSONGetRequest("/pumps");
  Utilities::writeDoc(0, moistureSensors, 1024);
  Utilities::writeDoc(1024, pumps, 2048);
}