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

void Utilities::writeFlash(DynamicJsonDocument &doc)
{
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
  EepromStream eepromStream(0, 1024); // Address 0, Size 1024 Bytes
  serializeJson(doc, eepromStream);
  EEPROM.commit();
}

DynamicJsonDocument Utilities::readFlash(int address)
{
  DynamicJsonDocument doc(1024);
  EepromStream eepromStream(address, 1024);
  deserializeJson(doc, eepromStream);

  for (int i = 0; i < doc.size(); i++)
  {
    String itemName = doc[i]["name"].as<String>();
    Serial.println(itemName);
  }

  return doc;
}
