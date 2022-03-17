#include <Services.h>

WiFiMulti wifiMulti;
// ESP8266WiFiMulti wifiMulti;

Services::Services()
{
}

void Services::setupWifiMulti()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void Services::setupWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to the WiFi network");
}

bool Services::getWifiMultiStatus()
{
  return wifiMulti.run() == WL_CONNECTED;
}

bool Services::getWifiStatus()
{
  return WiFi.status() == WL_CONNECTED;
}

void Services::doGetRequest(char url[])
{
  if (WiFi.status() == WL_CONNECTED)
  {

    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, otherwise 301 Code
    http.begin(url);

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
}

// https://randomnerdtutorials.com/esp32-http-get-post-arduino/
void Services::doPostRequest(char url[])
{
  if (WiFi.status() == WL_CONNECTED)
  {

    http.begin(url);
    http.addHeader("Content-Type", "text/plain");

    // Send HTTP POST request
    int httpResponseCode = http.POST("Huhu");

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
}

// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
DynamicJsonDocument Services::doJSONGetRequest(char url[])
{
  DynamicJsonDocument doc(2048);

  if (WiFi.status() == WL_CONNECTED)
  {
    // http.useHTTP10(true); // Error
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      // Parse response
      deserializeJson(doc, http.getStream());
      // ReadLoggingStream loggingStream(http.getStream(), Serial); // StreamUtils.h
      // deserializeJson(doc, loggingStream);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Disconnect
    http.end();
  }

  return doc;
}

void Services::startRestServer()
{
  // https://www.survivingwithandroid.com/esp32-rest-api-esp32-api-server/
  // WebServer webServer(80);
  // webServer.on("/", testFunc);
  // webServer.begin();

  /*
  // https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
  // Variable to store the HTTP request
  String header;
  unsigned long currentTime = millis();
  // Previous time
  unsigned long previousTime = 0;
  // Define timeout time in milliseconds (example: 2000ms = 2s)
  const long timeoutTime = 2000;
  WiFiServer wifiServer(80);
  WiFiClient client = wifiServer.available(); // Listen for incoming clients

  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  */
}

/*
void Services::runPump()
{
  Serial.println("Run Pump BUTTON pressed.");

  StaticJsonDocument<250> jsonDocument;
  char buffer[250];
  jsonDocument["Pump"] = "QR50E";
  serializeJson(jsonDocument, buffer);

  server.send(200, "application/json", "{}");
}
*/