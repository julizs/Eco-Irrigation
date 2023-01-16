#include <influxHelper.h>
#include <services.h>

/*
Use full param constructor to set fingerprint certificate to trust server
https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
InfluxDBClient InfluxHelper::client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
InfluxDBClient InfluxHelper::client;
Empty Constructor, create InfluxDBClient unconfigured instance, 
calling setConnectionParams is required (see Library), AFTER timeSync
*/
InfluxDBClient InfluxHelper::client;
Point p0("Environment Data");
Point p1("Plant Data");
Point p2("Irrigations");
Point p3("Water Flow");
Point p4("Power Usage");

/* Params
Timestamp (set by client) necessary to write data in Batches
If writePrecision is configured, client sets timestamps instead of server
Number of DataPoints varies, if User adds/removes Plants
Don't exceed bufferSize and flushInterval in one StateMachine run to prevent automatic Flush
Flush manually in TRANSMIT State
*/
bool InfluxHelper::setParameters()
{
  if(!client.validateConnection())
  {
    timeSync(TZ_INFO, "pool.ntp.org", "0.de.pool.ntp.org"); // time.nis.gov
    // Must be done AFTER timeSync
    client.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS).batchSize(64).bufferSize(128).retryInterval(5).maxRetryAttempts(10).flushInterval(180));
    client.setHTTPOptions(HTTPOptions().httpReadTimeout(10000).connectionReuse(true));
    Serial.println("Did Set Influx Options.");

    return true;
  } 
}

bool InfluxHelper::connectionEstablished()
{
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB at: ");
    Serial.println(client.getServerUrl());
    return true;
  }
  
  Serial.println(client.getLastErrorMessage());
  return false;
}

bool InfluxHelper::writeDataPoint(Point &p)
{
  // Write Datapoint to Buffer or DB (depending on Settings)
  if (!client.writePoint(p))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    return false;
  }

  // Success
  // Serial.println("Wrote Datapoint into Buffer");

  return true;
}

/*
Write all Datapoints from Buffer to InfluxDB, Critical
Try to reconnect if Fail since Datapoints in Buffer will be lost after Sleep
*/
bool InfluxHelper::writeBuffer()
{
  if (client.flushBuffer())
  {
    Serial.println("Successfully Wrote Buffer to InfluxDB.");
    // client.resetBuffer();
    return true;
  }
  else
  {
    Serial.println("Failed to write Buffer to InfluxDB.");
    // influxHelper.checkConnection();
    return false;
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