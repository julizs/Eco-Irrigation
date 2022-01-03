#include <services.h>
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