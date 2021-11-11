#include <Arduino.h>
#include <DHTesp.h>

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

#define VIN 3.3 // V power voltage, 3.3v in case of NodeMCU
#define R 10000 // light Resistor
#define DHTpin 0
#define analogPin A0

/* secure connection
InfluxDB client instance
use full param constructor to set the certificate or fingerprint to trust a server
https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#secure-connection
*/
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// create data point (= row) in a measurement (= table)
Point sensor("DHT_11");

DHTesp dht;


void setupSensors()
{
  dht.setup(DHTpin, DHTesp::DHT11); // DHT11
}

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

  setupSensors();

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

  /* batch
  batch = set of data points, sent at once (more efficient)
  https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino#batch-size
  batchsize dependant on number of data points per measurement and dashboard update rate
  */
  client.setWriteOptions(WriteOptions().batchSize(2));

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void getAmbientClimate()
{
  // DHT11
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  
  sensor.addField("common_humidity", humidity);
  sensor.addField("common_temperature", temperature);
}

void getAmbientLight()
{
  // https://www.geekering.com/categories/embedded-sytems/esp8266/ricardocarreira/esp8266-nodemcu-simple-ldr-luximeter/

  int rawVal = analogRead(analogPin);
  float Vout = float(rawVal) * (VIN / float(1023));// Conversion analog to voltage
  float RLDR = (R * (VIN - Vout))/Vout; // Conversion voltage to resistance
  int lux = 500/(RLDR/1000); // Conversion resitance to lumen

  sensor.addField("common_light", rawVal);
}


void loop() {

  delay(1000);
  
  //Clear fields for reusing the data point. Tags will remain untouched 
  sensor.clearFields();

  //Store measured field keys and field values into data point 
  getAmbientClimate();
  getAmbientLight();

  // sensor.addField("rssi", WiFi.RSSI());

  // Print what we are writing into point
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

   /*
   if(!client.isBufferEmpty())
   {
     Serial.println("Wrote data point into buffer.");
   }
   else 
   {
     Serial.println("Flushed Buffer, sent all points to server.");
   };
   */

  /* Query
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



