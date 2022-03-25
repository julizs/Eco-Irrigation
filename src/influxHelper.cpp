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
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    return true;
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    return false;
  }
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
Try to reconnect incase of write Fail or Datapoints in Buffer lost after Sleep
*/
void InfluxHelper::writeBuffer()
{
  int attempts = 0;

  while (!Services::getWifiStatus() && attempts < 3)
  {
    Services::setupWifi();
    delay(500);
    attempts++;
  }

  attempts = 0;

  while (!influxHelper.checkConnection() && attempts < 3)
  {
    delay(500);
    attempts++;
  }

  client.flushBuffer();
  Serial.println("Successfully Wrote Buffer to InfluxDB.");
}

FluxQueryResult InfluxHelper::doQuery(const char query[])
{
  FluxQueryResult result = client.query(query);

  // Iterate Result Cursor
  while (result.next())
  {
    long rssi = result.getValueByName("_value").getLong();
    Serial.println(rssi);
  }

  if (result.getError() != "")
  {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  result.close();

  return result;
}