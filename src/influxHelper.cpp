#include <influxHelper.h>
#include <services.h>

/* secure connection
InfluxDB client instance
use full param constructor to set the certificate or fingerprint to trust a server
https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
*/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// create data point (= row) in a measurement (= table)
// measurement name is string in point constructor?
// point obj gets reused for entering a row into the measurement
// 1 measurement/point per sensor or 1 for env data and 1 per plant data?
Point p0("Environment Data");
Point p1("Plant Data");
Point p2("Irrigations"); // "Plant Groups" ?

InfluxHelper::InfluxHelper()
{
  
}

void InfluxHelper::setupInflux()
{
  /* timestamp
  timestamp (set from client) necessary to write data in batches
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino
  async func. timeSync waits till time is synchronized
  configure writePrecision, now client is setting timestamps instead of server
  */
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

  /* Write Data as Batch
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#batch-size
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#buffer-handling-and-retrying
  batch = set of data points, sent at once (more efficient)
  batchsize dependant on number of data points per measurement and dashboard update rate
  bufferSize should be at least 2x batchSize
  with batchSize(0) or bufferSize(0): Often ssl connection error
  */
  client.setWriteOptions(WriteOptions().batchSize(4));
  client.setWriteOptions(WriteOptions().bufferSize(16));
  client.setWriteOptions(WriteOptions().flushInterval(30));
}

bool InfluxHelper::checkInfluxConnection()
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
  // If no Wifi signal, try to reconnect
  if ((WiFi.RSSI() == 0) && (Services::wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write data point into measurement/table or into buffer
  
  if (!client.writePoint(p)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  /*
  //Check Buffer
  if(client.isBufferEmpty())
  {
    Serial.println("Buffer is empty.");
  }
  else 
  {
    Serial.println("Wrote data point into buffer.");
  };
  */
}

void InfluxHelper::doQuery()
{
  /* Construct Query
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#querying
  Bei Bucketname Anfürungszeichen nötig (sonst als Identifier interpr.), 
  escapen des Anführungszeichen nötig sonst String Ende
  r.measurement ist value in measurement spalte, also data point name (z.B. wifi_status)
  https://docs.influxdata.com/influxdb/v1.8/concepts/key_concepts/
  */
  String query = "from (bucket: \"messdaten\")";
  query += "|> range(start: -1h)";
  query += "|> filter(fn: (r) => r._measurement == \"wifi_status\" and r._field == \"rssi\"";
  query += " and r.device == \"ESP8266\")";
  query += "|> min()";

  FluxQueryResult result = client.query(query);

  // Iterate Result Cursor
  while (result.next()) {
    // Get converted value for flux result column 'SSID'
    String ssid = result.getValueByName("SSID").getString();
    Serial.println();
    Serial.print("SSID: ");
    Serial.print(ssid);
  }

  if(result.getError() != "") {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  result.close();
}


/*
// Before: no sensor assignments, all sensors insert data into
// measurements but no plant tags yet
// User creates a new plant, no sensor assignments yet
void InfluxHelper::addPlant(char plantName[])
{
  p2.addTag("plantName", plantName);
}

// All old data should keep recent sensor/plant assignments before change
void InfluxHelper::removePlant(char plantName[])
{
  p2.clearFields();
  p2.clearTags();
  writeDataPoint(p2);
}

// First assign or reassign a SoilMoisture Sensor to a plant
void InfluxHelper::assignSoilMoistSensor(char plantName[], unsigned short multiplexerChannel)
{
  p2.clearFields();
  p2.clearTags();
  p2.addTag("plantName", plantName);
  p2.addField("soilMoistureSensor", multiplexerChannel);
  writeDataPoint(p2);
}
*/