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
Convert Irrigation InfluxDB Cursor to Vector for efficient checks of many items with only 1 DB Request
Used for quick solenoidValid() (iterate all solenoids, how much water output lately)
and evaluateIrrigation() (iterate all plants how much water received lately, irrigation necessary)
do not include "failed Irrigations" / Reports with invalid Solenoids (-1)

CAREFUL with FluxQueryResult, if printed before this Function then Cursor is empty
close() must be called after end of reading
Check reportInstructions() if Value is _field or _tag (String!)
*/
bool Utilities::cursorToVec(FluxQueryResult &cursor, std::vector<WaterPerSolenoid> &solenoids, uint8_t timePeriod)
{
  String solenoidValveTag, errorCodeTag;
  uint8_t solenoidValve, errorCode;
  uint16_t waterAmount;

  while (cursor.next())
  {
    // Tag(s)
    solenoidValveTag = cursor.getValueByName("solenoidValve").getString(); 
    errorCodeTag = cursor.getValueByName("errorCode").getString();
    solenoidValve = solenoidValveTag.toInt();
    errorCode = errorCodeTag.toInt();

    // Field(s)
    waterAmount = cursor.getValueByName("_value").getLong();

    // Only pick valid / executed Irrigations
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
      Serial.println("CursorToVec() Error: ");
      Serial.println(cursor.getError());
      return false;
    }
  }
  
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

  Serial.println("Total Water Distribution per Solenoid: ");

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

bool Utilities::containsDestination(String dest)
{
  for (int i = 0; i < manualTransitions.size(); i++)
  {
      if(manualTransitions.get(i).equals(dest))
      return true;
  }
  return false;
}

void Utilities::printDestinations()
{
  if(manualTransitions.size() > 0)
  {
    Serial.print("Manual Transitions List: ");
    for (int i = 0; i < manualTransitions.size(); i++)
    {
      Serial.print(manualTransitions.get(i));
      Serial.print(" ");
    }
    Serial.println(manualTransitions.size());
  }
  else
  {
    Serial.print("No User Actions.");
  }
}

bool Utilities::countTime(long beginMillis, uint8_t durationSec)
{
  return (millis() - beginMillis >= durationSec * 1000UL);
}