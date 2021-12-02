#include <services.h>

ESP8266WiFiMulti wifiMulti;

Services::Services()
{

}

void Services::SetupWifiMulti()
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

void Services::SetupWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
}

bool Services::GetWifiMultiStatus()
{
  return wifiMulti.run() == WL_CONNECTED;
}

bool Services::GetWifiStatus()
{
  return WiFi.status() == WL_CONNECTED;
}