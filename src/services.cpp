#include <Services.h>

HTTPClient Services::http;

void Services::setupWifi()
{
  long begin = millis();

  WiFi.softAP(AP_SSID);
  // WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // WiFi.onEvent(wifiEventHandler);

  Serial.println("Connecting to WiFi...");
  while (!Utilities::countTime(begin, 6));

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi.");
    // wifi_power_t power = WIFI_POWER_8_5dBm;
    // WiFi.setTxPower(WIFI_POWER_13dBm);
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
  http.setAuthorization("manip", "aristata89");
  http.addHeader("Content-Type", "application/json");

  // Send HTTP POST Req
  int httpResponseCode = http.POST(payload);

  snprintf(message, 128, "POST %s, HTTP Response code: %d ", url, httpResponseCode);
  // Serial.println(message);

  if (httpResponseCode > 0)
  {
    Serial.print(httpResponseCode + " ");
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
  // webServer.send(200, "text/plain", "200 All is Ok.");
}


/* 
"Passive" Handling of WebButtons
Serial.println(settings[0]["sleepDuration"].as<int>());
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
  JsonArray destinations = doc["transitions"].as<JsonArray>();
  if(!destinations.isNull())
  {
    for(int i = 0; i < destinations.size(); i++)
    {
      String dest = destinations.getElement(i).as<String>();
      if(dest.length() > 0) // Don't add " ", or wrong Count
        manualTransitions.add(dest);
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

  // Go to ACTION state if polling and action found
  // IDLE -> REQUEST -> ACTION -> TRANSMIT -> IDLE
  if(pumpActions.size() > 0 || irrActions.size() > 0)
  {
    Irrigation::writeInstructions();

    if(!Utilities::containsDestination("ACTION"))
    {
      manualTransitions.add("ACTION");
      manualTransitions.add("TRANSMIT");
    } 
  }
  

  /*
  Actuations (SolenoidValve, MatrixDisplay, ...)
  Stay in Action-State?
  */
  for(int i = 0; i < solActions.size(); i++)
  {
    JsonObject action = solActions[i];
    uint8_t channel = action["subject"];
    uint8_t state = action["isOpen"] == true? LOW : HIGH;
    // cistern1.driveSolenoid(channel, state);
    digitalWrite(relaisPins[channel], state);
  }

  /*
  JsonArray statusLights = actions["statusLights"].as<JsonArray>();
  StatusDisplay::handleActions(statusLights);
  */

  // Apply User Settings
  SLEEP_TYPE = doc["sleepType"].as<int>();
  SLEEP_DUR = doc["sleepDuration"].as<int>();

  // Reset User Actions for next Cycle
  Serial.print("Fetched User Actions, Resetting Doc: ");
  String emptyDoc = "{}";
  Services::doJSONPostRequest("/actions/reset", emptyDoc);
  
  return true;
  }