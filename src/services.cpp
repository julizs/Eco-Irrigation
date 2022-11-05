#include <Services.h>

// Definition of static Variables
WiFiMulti Services::wifiMulti;
HTTPClient Services::http;
WebServer Services::webServer(443);
// ip_addr_t Services::ip6_dns;

/*
https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiIPv6/WiFiIPv6.ino
Strg + Click WiFiEvent_t to see defined Event types
*/
void Services::wifiEventHandler(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_AP_START:
    WiFi.softAPsetHostname(AP_SSID);
    // Enable AP Ipv6
    WiFi.softAPenableIpV6();
    break;

  case SYSTEM_EVENT_STA_START:
    WiFi.setHostname(AP_SSID);
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    // Enable STA Ipv6
    WiFi.enableIpV6();
    break;
  case SYSTEM_EVENT_GOT_IP6:
    Serial.print("AP IPv6: ");
    Serial.println(WiFi.softAPIPv6());
    Serial.print("STA IPv6: ");
    Serial.println(WiFi.localIPv6());
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.println("STA Connected");
    Serial.print("STA IPv4: ");
    Serial.println(WiFi.localIP());
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("STA Disconnected");
    break;
  default:
    break;
  }
}

void Services::setupWifi()
{
  long begin = millis();

  // WiFi.softAP(AP_SSID);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // WiFi.onEvent(wifiEventHandler);

  Serial.println("Connecting to WiFi...");
  while (!Utilities::countTime(begin, 4));

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi.");
    // https://forum.arduino.cc/t/unable-to-change-esp32-s2-power/857338
    // wifi_power_t power = WIFI_POWER_8_5dBm;
    WiFi.setTxPower(WIFI_POWER_13dBm);
    Serial.print("Wifi Power: ");
    Serial.println(WiFi.getTxPower());
    return;
  }
  Serial.println("Could not connect to WiFi.");
}

bool Services::wifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

/*
WifiMulti: Multiple Wifi Hotspots in Fh
APSTA: Esp32Cam connect to main Esp32
*/
void Services::setupWifiMulti()
{
  long begin = millis();
  WiFi.mode(WIFI_MODE_APSTA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wifi...");
  wifiMulti.run();
  while (!Utilities::countTime(begin, 4));

  if (!wifiMulti.run())
  {
    // critErrMessage = "Final fail to connect to WiFi.";
    critErrCode = 1;
  }
}

bool Services::wifiMultiConnected()
{
  return wifiMulti.run() == WL_CONNECTED;
}


String Services::doGetRequest(char const url[])
{
  char message[128];
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Needed, or 301 Code
  http.begin(url);

  // Send HTTP GET Req
  int httpResponseCode = http.GET();

  snprintf(message, 128, "GET %s Response Code: %d", url, httpResponseCode);
  Serial.println(message);

  String response = "";

  if (httpResponseCode > 0)
    response = http.getString();

  http.end();

  return response;
}

/*
Funcs are responsible for packaging data as json
PostRequest accepts json as input param
https://randomnerdtutorials.com/esp32-http-get-post-arduino/
*/
void Services::doJSONPostRequest(char const endpoint[], String &payload)
{
  char url[50] = "", message[128];
  strcat(url, baseUrl);
  strcat(url, endpoint);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Send HTTP POST Req
  int httpResponseCode = http.POST(payload);

  snprintf(message, 128, "POST %s, HTTP Response code: %d ", url, httpResponseCode);
  // Serial.println(message);

  if (httpResponseCode > 0)
  {
    Serial.println(http.getString()); // Print payload
  }

  http.end();
}

// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
void Services::doJSONGetRequest(char const endpoint[], DynamicJsonDocument &doc)
{
  char url[50] = "", message[128];
  strcat(url, baseUrl);
  strcat(url, endpoint);

  // http.useHTTP10(true); // Error
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.begin(url);
  int httpResponseCode = http.GET();

  snprintf(message, 128, "GET %s, HTTP Response code: %d ", url, httpResponseCode);
  // Serial.println(message);

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

/* "Active" Handling of WebButtons
http://192.168.178.86:443/
http://192.168.178.86:443/solenoidValve
http://95.222.24.18:443/solenoidValve (geht nicht)
https://en.avm.de/service/knowledge-base/dok/FRITZ-Box-7369-int/1089_Identifying-the-address-range-of-the-IPv4-address-for-the-internet-connection/
https://avm.de/service/wissensdatenbank/dok/FRITZ-Box-7590/1083_Kein-Zugriff-uber-Portfreigabe-auf-FRITZ-Box-Heimnetz/
The FRITZ!Box does not have a public IPv4 address if the message
"FRITZ!Box uses a DS Lite tunnel" is displayed. In this case,
the FRITZ!Box cannot be accessed from the internet over IPv4

FritzBox Interface: Three Dots top right, Advanced View
Internet, Online Monitor, Ipv4/Ipv6 Addresses (FritzBox uses a DS Lite Tunnel?)
Browser: https://[2a02:8070:e300:0:3c4d:e882:749c:f059]/ (FritzBox Interface shows)
CMD: ping 2a02:8070:e300:0:3c4d:e882:749c:f059
Reach via Ipv6:
Router AND devices (e.g. Esp32) have to be connected via Ipv6

https://www.survivingwithandroid.com/esp32-rest-api-esp32-api-server/
*/
void Services::startRestServer()
{
  webServer.on("/", rootEndpoint);
  webServer.on("/solenoidValve", []()
               {
      // digitalWrite(Relais[0], LOW);
      digitalWrite(Relais[1], LOW);
      webServer.send(200, "text/plain", "Toggled Solenoid Valve"); });

  webServer.begin();

  Serial.print("Web Server started on Core: ");
  Serial.println(xPortGetCoreID());
}

/* "Passive" Handling of WebButtons
Serial.println(settings[0]["sleepDuration"].as<int>());
Save JsonObjects globally or locally to release after exit / save mermory?
Convert JsonObjects/JsonArrays to local Datastructures (e.g. instr Vec)? 

https://arduinojson.org/v6/how-to/reuse-a-json-document/
IMPORTANT: difference as/to<JsonArray> and as/to<JsonObject>
Check settings schema in mongoose whether
Attributes are JsonObject {}, e.g. "actions" or JsonArray []
*/
bool Services::readSettings()
{
  DynamicJsonDocument settings(1024);
  Services::doJSONGetRequest("/settings", settings);
  
  if(settings.isNull())
    return false;

  // settings is a mongoDB Collection with only 1 Document inside [{}]
  JsonArray collection = settings.as<JsonArray>();
  JsonObject doc = collection.getElement(0).as<JsonObject>();

  // Handle User Actions
  JsonArray destinations = doc["transitionTo"].as<JsonArray>();
  if(!destinations.isNull())
  {
    for(int i = 0; i < destinations.size(); i++)
    {
      String dest = destinations.getElement(i).as<String>();
      if(dest.length() > 0) // Don't add " ", or wrong Count
        transDestinations.add(dest);
    }
  }

  JsonObject actions = doc["actions"].as<JsonObject>();
  JsonArray irrActions = actions["irrigations"].as<JsonArray>();
  JsonArray pumpActions = actions["pumps"].as<JsonArray>();
  JsonArray solActions = actions["solenoidValves"].as<JsonArray>();

  if(pumpActions.size() > 0)
    Irrigation::createInstructions(pumpActions, Irrigation::instructions);
  if(irrActions.size() > 0)
    Irrigation::createInstructions(irrActions, Irrigation::instructions);

  if(pumpActions.size() > 0 || irrActions.size() > 0)
  {
    Irrigation::writeInstructions();

    /*
    Handle this in REST Endpoint instead?
    Directly go to ACTION state to execute Actions
    IDLE -> REQUEST -> ACTION -> TRANSMIT -> IDLE
    */
    if(!Utilities::containsDestination("ACTION"))
    {
      transDestinations.add("ACTION");
      transDestinations.add("TRANSMIT");
    } 
  }
  

  /*
  SolenoidValve, MatrixDisplay, ...
  Also only do in Action-State? Duration?
  */
  for(int i = 0; i < solActions.size(); i++)
  {
    JsonObject action = solActions[i];
    uint8_t channel = action["subject"];
    uint8_t state = action["isOpen"] == true? LOW : HIGH;
    // cistern1.driveSolenoid(channel, state);
    digitalWrite(Relais[channel], state);
  }

  /*
  JsonArray statusLights = actions["statusLights"].as<JsonArray>();
  StatusDisplay::handleActions(statusLights);
  */

  // Handle Settings
  SLEEP_TYPE = doc["sleepType"].as<int>();
  SLEEP_DUR = doc["sleepDuration"].as<int>();

  /*
  for (JsonPair keyValue : obj) {
    Serial.println(keyValue.key().c_str()); 
  } 
  */

  // Reset Actions for next Iteration
  Serial.println("(Resetting User Actions): ");
  String emptyDoc = "{}";
  Services::doJSONPostRequest("/settings/reset", emptyDoc);
  
  return true;
  }