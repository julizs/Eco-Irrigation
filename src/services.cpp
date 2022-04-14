#include <Services.h>

// Definition of static Variables
WiFiMulti Services::wifiMulti;
HTTPClient Services::http;
WebServer Services::webServer(443);

void Services::setupWifiMulti()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  long begin = millis();
 
  while (!wifiMultiConnected() && !countTime(begin, 4))
  {
  }
  Serial.println();

  if (!wifiMultiConnected())
  {
    ESP.restart();
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

  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, otherwise 301 Code
  http.begin(url);

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  Serial.print("Requesting ");
  Serial.print(url);
  Serial.print(" : ");

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.print(httpResponseCode);
    Serial.print(" ");
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

// https://randomnerdtutorials.com/esp32-http-get-post-arduino/
void Services::doPostRequest(char const endpoint[])
{
  if (!wifiMultiConnected())
  {
    setupWifiMulti();
  }

  char requestUrl[50] = "";
  strcat(requestUrl, baseUrl);
  strcat(requestUrl, endpoint);
  Serial.print("POST Request to: ");
  Serial.println(requestUrl);

  http.begin(requestUrl);
  http.addHeader("Content-Type", "application/json");

  // Send HTTP POST request
  int httpResponseCode = http.POST("{}");

  Serial.print("Requesting ");
  Serial.print(requestUrl);
  Serial.print(" : ");

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.print(httpResponseCode);
    Serial.print(" ");
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

// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
void Services::doJSONGetRequest(char const endpoint[], DynamicJsonDocument &doc)
{
  if (!wifiMultiConnected())
  {
    setupWifiMulti();
  }

  char requestUrl[50] = "";
  strcat(requestUrl, baseUrl);
  strcat(requestUrl, endpoint);
  Serial.print("GET Request to: ");
  Serial.println(requestUrl);

  // http.useHTTP10(true); // Error
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.begin(requestUrl);
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

/*
Passive Handling of WebButtons
Read User Commands and Settings, but take Actions later in Action State
*/
/*
void Services::handleWebButtons()
{
  DynamicJsonDocument commands = Services::doJSONGetRequest("/commands");

  // Solenoid
  int relaisChannel = commands[0]["SolenoidValve"][0];
  bool relaisState = commands[0]["SolenoidValve"][1];

  // Irrigation (Plant or Pump)
  const char *irrSubject = commands[0]["Irrigation"][0];
  int irrAmount = commands[0]["Irrigation"][1];

  // Status Light
  const char *display = commands[0]["StatusLight"][0];
  int displayContent = commands[0]["StatusLight"][1];

  // Reset all Commands...
}
*/

bool Services::countTime(long begin, uint8_t duration)
{
  return (millis() - begin >= duration * 1000UL);
}