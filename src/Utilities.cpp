#include "Utilities.h"

uint32_t Utilities::stateBeginMillis = 0;

uint8_t Utilities::scanI2CBus(TwoWire *wire)
{
  char message[64];
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

  snprintf(message, 64, "Clock: %d, Num of Devices: %d, Error:%s",
           wire->getClock(), cnt, wire->getErrorText(wire->lastError()));
  Serial.println(message);

  return cnt;
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
In REQUEST State
Convert Reports/Irrigations to Vector for quick validSolenoid Checks later

CAREFUL with FluxQueryResult, if printed before this Function then Cursor is empty
close() must be called after end of reading

Check reportInstructions() if Value is _field or _tag (String!)
This Vector is built based on Reports and used for checking waterLimits per Solenoid,
so do not include "failed Irrigations" / Reports with invalid Solenoids (-1)
*/
bool Utilities::cursorToVec(FluxQueryResult &cursor, std::vector<WaterPerSolenoid> &solenoids, uint8_t timePeriod)
{
  String solenoidValveTag, errorCodeTag;
  int solenoidValve, errorCode;
  uint16_t waterAmount;

  while (cursor.next())
  {
    solenoidValveTag = cursor.getValueByName("solenoidValve").getString(); // Tag(s)
    errorCodeTag = cursor.getValueByName("errorCode").getString();
    solenoidValve = solenoidValveTag.toInt();
    errorCode = errorCodeTag.toInt();
    waterAmount = cursor.getValueByName("_value").getLong();

    // Only pick valid/actual Irrigations
    if(errorCode == 0 && solenoidValve != -1)
    {
      WaterPerSolenoid sol;
      sol.solenoidValve = solenoidValve;
      sol.waterAmount = waterAmount;
      sol.timePeriod = timePeriod;
      solenoids.push_back(sol);
    }

    if (cursor.getError() != "")
    {
      Serial.println("Vector write Error: ");
      Serial.println(cursor.getError());
    }
  }
  // Serial.println("Wrote FluxCursor to Vector.");
  cursor.close();

  return true;
}

/*
How to print all Tags/Fields?
*/
void Utilities::printCursor(FluxQueryResult &cursor)
{
  Serial.println("Printing Cursor");

  while (cursor.next())
  {
    Serial.println(cursor.getValueByName("solenoidValve").getLong());
    Serial.println(cursor.getValueByName("_value").getLong());

    if (cursor.getError() != "")
    {
      Serial.println("Vector write Error: ");
      Serial.println(cursor.getError());
    }
  }
}

void Utilities::printSolenoids(std::vector<WaterPerSolenoid> &solenoids)
{
  char message[64];

  Serial.println("Recent Irrigations: ");

  if (solenoids.size() > 0)
  {
    for (auto const &sol : solenoids)
    {
      snprintf(message, 64, "Solenoid Valve: %d, Water Amount: %dml, Time Period: %dh",
               sol.solenoidValve, sol.waterAmount, sol.timePeriod);
      Serial.println(message);
    }
  }
  else
  {
    Serial.println("None.");
  }
}

bool Utilities::countTime(long begin, uint8_t duration)
{
  return (millis() - begin >= duration * 1000UL);
}