#include <influxHelper.h>
#include <services.h>

// Use full param constructor to set the certificate or fingerprint to trust a server
// https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point p0("Environment Data");
Point p1("Plant Data");
Point p2("Irrigations");

void InfluxHelper::setParameters()
{
  /* Timestamp
  timestamp (set from client) necessary to write data in batches
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino
  If writePrecision is configured, client sets timestamps instead of server
  */
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

  // Use high batchSize and do manual flushBuffer, because
  // Number of DataPoints varies, since User can add Plants
  client.setWriteOptions(WriteOptions().batchSize(100));
  // client.setWriteOptions(WriteOptions().bufferSize(2)); // Don't use
  // client.setWriteOptions(WriteOptions().flushInterval(30));

  Serial.println("Did Setup Influx Options.");
}

bool InfluxHelper::checkConnection()
{
  if (!Services::getWifiStatus())
  {
    Services::setupWifi();
  }

  int attempts = 0;

  while (attempts < 3)
  {
    long begin = millis();
    Serial.println("Trying to connect to InfluxDB.");
    if (client.validateConnection())
      ;
    {
      Serial.print("Connected to: ");
      Serial.println(client.getServerUrl());
      return true;
    }
    while (!Services::countTime(begin, 2))
    {
    }
    attempts++;
  }

  Serial.print("InfluxDB connection failed: ");
  Serial.println(client.getLastErrorMessage());
  return false;
}

void InfluxHelper::writeDataPoint(Point &p)
{
  if (client.isBufferEmpty())
  {
    Serial.println("Buffer is empty.");
  }
  else
  {
    Serial.println("Buffer is not empty.");
  }

  // Write Datapoint into Buffer (or to Database if Buffer Overflow)
  if (!client.writePoint(p))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

/*
Write all Datapoints from Buffer to InfluxDB, Critical
Try to reconnect if Fail since Datapoints in Buffer will be lost after Sleep
*/
void InfluxHelper::writeBuffer()
{
  int attempts = 0;

  while (attempts < 3)
  {
    if (client.flushBuffer())
    {
      Serial.println("Successfully Wrote Buffer to InfluxDB.");
      return;
    }
    else
    {
      // Only call if inital Attempt fails, validateConnection takes long
      Serial.println("Failed to write Buffer to InfluxDB.");
      influxHelper.checkConnection();
      attempts++;
    }
  }
}

FluxQueryResult InfluxHelper::doQuery(const char query[])
{
  FluxQueryResult result = client.query(query);

  return result;
}

/*
// Called by Irrigation.cpp and Pump.cpp (on Button Press)
// Check per per SolenoidValve/relaisChannel
int InfluxHelper::solenoidReleasedWater(uint8_t relaisChannel)
{
  char rC[20] = "relaisChannel == ";
  itoa(relaisChannel, rC, 10);

  // Plants, SolenoidValve/relaisChannel and Pump are available as Query Tags
  char query[200] = "from (bucket: \"messdaten\") |> range(start: -1h) |> filter(fn: (r) => r._measurement == \"Irrigations\" and ";
  strcat(query, rC);
  strcat(query, " and r._field == \"pumpedWaterML\" and r.device == \"ESP32\") |> min()");

  FluxQueryResult cursor = influxHelper.doQuery(query);
  long pumpedWaterML = -1;

  // Iterate Result Cursor
  while (cursor.next())
  {
    pumpedWaterML = cursor.getValueByName("pumpedWaterML").getLong();
    Serial.println(pumpedWaterML);
  }

  if (cursor.getError() != "")
  {
    Serial.print("Query result error: ");
    Serial.println(cursor.getError());
  }

  cursor.close();

  return 0;
}
*/