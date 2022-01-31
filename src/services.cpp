#include <Services.h>
#include <WiFiMulti.h>

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

void Services::doGetRequest()
{
 if(WiFi.status()== WL_CONNECTED){

      HTTPClient http;
      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, otherwise 301 Code
      http.begin("https://juli.uber.space/node/plants");
      
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

// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
DynamicJsonDocument Services::doJSONGetRequest()
{
  DynamicJsonDocument doc(2048);

  if(WiFi.status()== WL_CONNECTED)
    {
      HTTPClient http;

      // Send request
      // http.useHTTP10(true); // Error
      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      http.begin("https://juli.uber.space/node/plants");
      int httpResponseCode = http.GET();

      if (httpResponseCode>0) 
      {
        // Parse response
        deserializeJson(doc, http.getStream());
        //ReadLoggingStream loggingStream(http.getStream(), Serial); // StreamUtils.h
        //deserializeJson(doc, loggingStream);

        /*
        // Read values
        Serial.println(doc[0].as<String>());
        Serial.println(doc[0]["plantName"].as<String>());

        for(int i = 0; i < doc.size(); i++)
        {
          Serial.println(doc[i]["plantName"].as<String>());
        }
        */
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