#include <Arduino.h>

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "FRITZ!Box 7430 ED"
#define WIFI_PASSWORD "49391909776212256241"
#define INFLUXDB_URL "https://juli.uber.space"
#define INFLUXDB_TOKEN "njMUu2TT8HUi7rmhWj_tq75nCkIIhKuODAlRoR4w7GNoLVHGiFvL26VAZ-fQ4w2sQennJPCH9FsVbch3_r6zaA=="
#define INFLUXDB_ORG "private"
#define INFLUXDB_BUCKET "messdaten"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Einen Data Point (= Zeile) in einem Measurement (= Tabelle) erzeugen
Point sensor("wifi_status");


void setup() {

  delay(200);
  Serial.begin(9600);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Add tags to Data Point
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // timestamp (set from client) necessary to write data in batches
  // https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino
  // async func. timeSync waits till time is synchronized
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  // configure writePrecision, now client is setting timestamps instead of server
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

  // batch = set of data points, sent at once (more efficient)
  // https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#batch-size
  // batchsize dependant on number of data points per measurement and dashboard update rate
  client.setWriteOptions(WriteOptions().batchSize(5));

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}


void loop() {

  delay(1000);

  // Clear fields for reusing the data point. Tags will remain
  sensor.clearFields();

  // Store measured field key and field value into data point 
  // (e.g. rssi of current connected network)
  sensor.addField("rssi", WiFi.RSSI());

  // Print what are we exactly writing
  // Serial.print("Writing (into measurement/table): ");
  // Serial.println(sensor.toLineProtocol());

  // If no Wifi signal, try to reconnect it
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // write data point into measurement/table or into buffer
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

   
   if(!client.isBufferEmpty())
   {
     Serial.println("Wrote data point into buffer.");
   }
   else 
   {
     Serial.println("Flushed Buffer, sent all points to server.");
   };

  /*
  // Query
  // Bei Bucketname Anfürungszeichen nötig (sonst als Identifier interpr.), 
  // escapen des Anführungszeichen nötig sonst String Ende
  String query = "from (bucket: \"messdaten\")";
  query += "|> range(start: -1h)";
  query += "|> filter(fn: (r) => r._measurement == \"wifi_status\" and r._field == \"rssi\"";
  query += " and r.device == \"ESP8266\")";
  query += "|> min()";

  String query2 = "SELECT * FROM \"messdaten\"";
  FluxQueryResult result = client.query(query);

  // Auf Cursor iterieren
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
  */
}


  /*
  // Construct a Flux query
  // Query will find the worst RSSI for last hour for each connected WiFi network with this device
  String query = "from(bucket: \\"" INFLUXDB_BUCKET "\\") |> range(start: -1h) |> filter(fn: (r) => r._measurement == \\"wifi_status\\" and r._field == \\"rssi\\"";
  query += " and r.device == \\""  DEVICE  "\\")";
  query += "|> min()";

  // Print ouput header
  Serial.print("==== ");
  Serial.print(selectorFunction);
  Serial.println(" ====");

  // Print composed query
  Serial.print("Querying with: ");
  Serial.println(query);

  // Send query to the server and get result
  FluxQueryResult result = client.query(query);

  // Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    // Get converted value for flux result column 'SSID'
    String ssid = result.getValueByName("SSID").getString();
    Serial.print("SSID '");
    Serial.print(ssid);

    Serial.print("' with RSSI ");
    // Get converted value for flux result column '_value' where there is RSSI value
    long value = result.getValueByName("_value").getLong();
    Serial.print(value);

    // Get converted value for the _time column
    FluxDateTime time = result.getValueByName("_time").getDateTime();

    // Format date-time for printing
    // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
    String timeStr = time.format("%F %T");

    Serial.print(" at ");
    Serial.print(timeStr);

    Serial.println();
  }

  // Check if there was an error
  if(result.getError() != "") {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  // Close the result
  result.close();
  */



