#include <Services.h>

WiFiMulti wifiMulti;
//ESP8266WiFiMulti wifiMulti;

Services::Services()
{

}

void Services::setupWifiMulti()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void Services::setupWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
 
  while (WiFi.status() != WL_CONNECTED) {
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
 if(WiFi.status()== WL_CONNECTED){

      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, otherwise 301 Code
      http.begin(url);
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
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
 if(WiFi.status()== WL_CONNECTED){

      http.begin(url);
      http.addHeader("Content-Type", "text/plain");
      
      // Send HTTP POST request
      int httpResponseCode = http.POST("Huhu");
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      else {
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

  if(WiFi.status()== WL_CONNECTED)
    {
      // http.useHTTP10(true); // Error
      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      http.begin(url);
      int httpResponseCode = http.GET();

      if (httpResponseCode>0) 
      {
        // Parse response
        deserializeJson(doc, http.getStream());
        //ReadLoggingStream loggingStream(http.getStream(), Serial); // StreamUtils.h
        //deserializeJson(doc, loggingStream);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }  

      // Disconnect
      http.end();
    }

    return doc;
}