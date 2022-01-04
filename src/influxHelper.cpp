#include <influxHelper.h>
#include <services.h>

/* secure connection
InfluxDB client instance
use full param constructor to set the certificate or fingerprint to trust a server
https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
*/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// create data point (= row) in a measurement (= table)
Point p0("Wifi_Status");
Point p1("DHT_22");
Point p2("VL530X");
Point p3("TSL2591");

InfluxHelper::InfluxHelper()
{
  
}

void InfluxHelper::setupInflux()
{
  // Add tags to data point
  p0.addTag("device", DEVICE);
  p0.addTag("SSID", WiFi.SSID());

  /* timestamp
  timestamp (set from client) necessary to write data in batches
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino
  async func. timeSync waits till time is synchronized
  configure writePrecision, now client is setting timestamps instead of server
  */
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

  /* Write Data as Batch
  batch = set of data points, sent at once (more efficient)
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#batch-size
  batchsize dependant on number of data points per measurement and dashboard update rate
  */
  client.setWriteOptions(WriteOptions().batchSize(2));
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

void InfluxHelper::writeDataPoint(Point p, char* key, int value)
{
  //Clear fields for reusing the data point. Tags will remain untouched 
  p.clearFields();

  //Store measured field keys and field values into data point
  //sensor.addField("common_humidity", m.humidity);
  //sensor.addField("common_temperature", m.temperature);
  p.addField(key, value);

  /* Print to be written point into console
  // Serial.print("Writing (into measurement/table): ");
  // Serial.println(p.toLineProtocol());
  */

  // If no Wifi signal, try to reconnect
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write data point into measurement/table or into buffer
  if (!client.writePoint(p)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

   /* Check Buffer
   if(!client.isBufferEmpty())
   {
     Serial.println("Wrote data point into buffer.");
   }
   else 
   {
     Serial.println("Flushed Buffer, sent all points to server.");
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

