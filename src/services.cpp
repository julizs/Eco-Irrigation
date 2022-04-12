#include <Services.h>

// Definition of static Variables
WiFiMulti Services::wifiMulti;
HTTPClient Services::http;

void Services::setupWifiMulti()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  long begin = millis();
  // Try reconnecting for 8 Seconds
  while (wifiMulti.run() != WL_CONNECTED && !countTime(begin, 8))
  {
  }
  Serial.println();

  if (wifiMulti.run() != WL_CONNECTED)
  {
    ESP.restart();
  }
}

bool Services::getWifiMultiStatus()
{
  return wifiMulti.run() == WL_CONNECTED;
}

void Services::doGetRequest(char const url[])
{
  if (!getWifiMultiStatus())
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
  char requestUrl[50] = "";
  strcat(requestUrl, baseUrl);
  strcat(requestUrl, endpoint);
  Serial.print("POST Request to: ");
  Serial.println(requestUrl);

  if (!getWifiMultiStatus())
  {
    setupWifiMulti();
  }

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
DynamicJsonDocument Services::doJSONGetRequest(char const endpoint[]) // char endpoint[]
{
  char requestUrl[50] = "";
  strcat(requestUrl, baseUrl);
  strcat(requestUrl, endpoint);
  Serial.print("GET Request to: ");
  Serial.println(requestUrl);

  DynamicJsonDocument doc(2048);

  if (!getWifiMultiStatus())
  {
    setupWifiMulti();
  }

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

  return doc;
}

bool Services::countTime(long begin, uint8_t duration)
{
  return (millis() - begin >= duration * 1000UL);
}