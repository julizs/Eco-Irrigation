#include <Arduino.h>
#include <Wire.h>
#include <climate.h>
#include <soilmoisture.h>
#include <fotoresistor.h>
#include <pins.h>
#include <services.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define INFLUXDB_URL "https://juli.uber.space"
#define INFLUXDB_TOKEN "njMUu2TT8HUi7rmhWj_tq75nCkIIhKuODAlRoR4w7GNoLVHGiFvL26VAZ-fQ4w2sQennJPCH9FsVbch3_r6zaA=="
#define INFLUXDB_ORG "private"
#define INFLUXDB_BUCKET "messdaten"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"


Climate climate1(4);
SoilMoisture soilMoisture1(550);
Fotoresistor fotoresistor1(10000, 3.3, analogPin);
Services services;

/* secure connection
InfluxDB client instance
use full param constructor to set the certificate or fingerprint to trust a server
https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
*/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// create data point (= row) in a measurement (= table)
Point sensor("DHT_11");


void CheckInfluxConnection()
{
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void SetupInflux()
{
  // Add tags to data point
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

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

void setup() {

  delay(200);

  Serial.begin(9600);

  services.SetupWifi();

  // SetupSensors();
  climate1.InitialiseDHT();

  SetupInflux();

  CheckInfluxConnection();
}

void WriteDataPoint()
{
  //Clear fields for reusing the data point. Tags will remain untouched 
  sensor.clearFields();

  //Store measured field keys and field values into data point
  //sensor.addField("common_humidity", climate1.MeasureHumidityDHT());
  //sensor.addField("common_temperature", climate1.MeasureTemperatureDHT());
  DHTdata m = climate1.MeasureDHT();
  sensor.addField("common_humidity", m.humidity);
  sensor.addField("common_temperature", m.temperature);
  sensor.addField("common_light", fotoresistor1.measureLight());
  sensor.addField("rssi", WiFi.RSSI());

  /* Print into Console what we are writing into point
  // Serial.print("Writing (into measurement/table): ");
  // Serial.println(sensor.toLineProtocol());
  */

  // If no Wifi signal, try to reconnect
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write data point into measurement/table or into buffer
  if (!client.writePoint(sensor)) {
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

void DoQuery()
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


void loop() {

  delay(2000);
  WriteDataPoint();
}



