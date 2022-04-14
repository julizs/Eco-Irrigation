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
  /* Params
  Timestamp (set by client) necessary to write data in Batches
  If writePrecision is configured, client sets timestamps instead of server
  Number of DataPoints varies, if User adds/removes Plants
  Prevent automatic Flush by setting bufferSize and flushInterval high
  Flush manually in TRANSMIT State
  */
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));
  client.setWriteOptions(WriteOptions().batchSize(128));
  client.setWriteOptions(WriteOptions().bufferSize(256));
  // client.setWriteOptions(WriteOptions().flushInterval(180));
  // client.setWriteOptions(WriteOptions().retryInterval(3));
  client.setWriteOptions(WriteOptions().maxRetryAttempts(5));
  client.setWriteOptions(WriteOptions().maxRetryInterval(30));
  // Connection refused:
  // client.setHTTPOptions(HTTPOptions().connectionReuse(true));
  // client.setHTTPOptions(HTTPOptions().httpReadTimeout(10000));

  Serial.println("Did Set Influx Options.");
}

bool InfluxHelper::checkConnection()
{
  if (!Services::wifiMultiConnected())
  {
    Services::setupWifiMulti();
  }

  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB at: ");
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
    Serial.println("Buffer was flushed and is empty.");
  }
  else
  {
    Serial.println("Wrote Datapoint into Buffer");
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
  if (client.flushBuffer())
  {
    Serial.println("Successfully Wrote Buffer to InfluxDB.");
    // client.resetBuffer();
    return;
  }
  else
  {
    // Only call if inital Attempt fails, validateConnection takes long
    Serial.println("Failed to write Buffer to InfluxDB.");
    influxHelper.checkConnection();
  }
}

FluxQueryResult InfluxHelper::doQuery(const char query[])
{
  // Serial.println("Querying InfluxDB...");
  FluxQueryResult result = client.query(query);

  if (result.getError() != "")
  {
    Serial.println("Query Error: ");
    Serial.println(result.getError());
  }

  return result;
}