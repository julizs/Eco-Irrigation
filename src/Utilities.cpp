#include "Utilities.h"

std::vector<MlPerSolenoid> Utilities::ml2h, ml24h;

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
FluxQueryResult to Vector
*/
bool Utilities::writeVector(FluxQueryResult &cursor, std::vector<MlPerSolenoid> &solenoids)
{
  uint8_t solenoidValve;
  uint16_t waterAmount;

    while (cursor.next())
    {
        solenoidValve = cursor.getValueByName("solenoidValve").getLong();
        waterAmount = cursor.getValueByName("_value").getLong();
        bool exists = false;

        for (auto &sol : solenoids)
        {
            if (sol.solenoidValve == solenoidValve)
            {
                sol.waterAmount += waterAmount;
                exists = true;
            }
        }
        if (!exists)
        {
            MlPerSolenoid sol;
            sol.solenoidValve = solenoidValve;
            sol.waterAmount = waterAmount;
            solenoids.push_back(sol);
        }

        if (cursor.getError() != "")
        {
            Serial.println("Vector write Error: ");
            Serial.println(cursor.getError());
        }
    }
    Serial.println("Vector write Success.");
    return true;
}

bool Utilities::countTime(long begin, uint8_t duration)
{
  return (millis() - begin >= duration * 1000UL);
}