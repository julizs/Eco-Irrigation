#include <Services.h>

// Definition of static Variables
WiFiMulti Services::wifiMulti;
HTTPClient Services::http;
WebServer Services::webServer(443);

void Services::setupWifiMulti()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  long begin = millis();
 
  Serial.print("Connecting to Wifi.");
  while (!wifiMultiConnected() && !Utilities::countTime(begin, 8))
  {}

  if (!wifiMultiConnected())
  {
    critErrCode = 1;
  }
}

bool Services::wifiMultiConnected()
{
  return wifiMulti.run() == WL_CONNECTED;
}

void Services::doGetRequest(char const url[])
{
  if (!wifiMultiConnected())
  {
    setupWifiMulti();
  }

  char message[128];
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, otherwise 301 Code
  http.begin(url);

  // Send HTTP GET Req
  int httpResponseCode = http.GET();

  snprintf(message, 128,"GET %s Response Code: %d", url, httpResponseCode);
  Serial.println(message);

  if (httpResponseCode > 0)
  { 
    Serial.println(http.getString());
  }
  
  http.end();
}

// https://randomnerdtutorials.com/esp32-http-get-post-arduino/
void Services::doPostRequest(char const endpoint[])
{
  if (!wifiMultiConnected())
  {
    setupWifiMulti();
  }

  char url[50] = "", message[128];
  strcat(url, baseUrl);
  strcat(url, endpoint);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Send HTTP POST Req
  int httpResponseCode = http.POST("{}");

  snprintf(message, 128, "POST %s, HTTP Response code: %d ", url, httpResponseCode);
  Serial.println(message);

  if (httpResponseCode > 0)
  {
    Serial.println(http.getString()); // Print payload
  }

  http.end();
}

// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
void Services::doJSONGetRequest(char const endpoint[], DynamicJsonDocument &doc)
{
  if (!wifiMultiConnected())
  {
    setupWifiMulti();
  }

  char url[50] = "", message[128];
  strcat(url, baseUrl);
  strcat(url, endpoint);

  // http.useHTTP10(true); // Error
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.begin(url);
  int httpResponseCode = http.GET();

  snprintf(message, 128, "GET %s, HTTP Response code: %d ", url, httpResponseCode);
  Serial.println(message);

  if (httpResponseCode > 0)
  {
    deserializeJson(doc, http.getStream()); // Parse response
    // ReadLoggingStream loggingStream(http.getStream(), Serial); // StreamUtils.h
    // deserializeJson(doc, loggingStream);
  }

  http.end();
}

void Services::rootEndpoint()
{
  Serial.print("Web Server Clients handled on Core: ");
  Serial.println(xPortGetCoreID());
  webServer.send(200, "text/plain", "200 All is Ok.");
}

/*
Active Handling of WebButtons
*/
void Services::startRestServer()
{
  // https://www.survivingwithandroid.com/esp32-rest-api-esp32-api-server/
  webServer.on("/", rootEndpoint);
  webServer.on("/solenoidValve", []() {
      // digitalWrite(Relais[0], LOW);
      digitalWrite(Relais[1], LOW);
      webServer.send(200, "text/plain", "Toggled Solenoid Valve");
  });

  webServer.begin();

  Serial.print("Web Server started on Core: ");
  Serial.println(xPortGetCoreID());
}