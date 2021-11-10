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

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("wifi_status");


void setup() {
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Add tags
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

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
  // Clear fields for reusing the point. Tags will remain untouched
  sensor.clearFields();

  // Store measured value into point
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  // If no Wifi signal, try to reconnect it
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  //Wait 3s
  Serial.println("Wait 10s");
  delay(3000);



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
}


