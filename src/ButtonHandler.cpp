#include <ButtonHandler.h>

// Definition of static Variable
WebServer ButtonHandler::webServer(443);

void ButtonHandler::root()
{
  Serial.println("Web Server Clients handled on Core: ");
  Serial.println(xPortGetCoreID());

  webServer.send(200, "text/plain", "200 All is Ok.");
}

void ButtonHandler::startRestServer()
{
  // https://www.survivingwithandroid.com/esp32-rest-api-esp32-api-server/
  webServer.on("/", root);
  webServer.on("/solenoidValve", []() {
      // digitalWrite(Relais[0], LOW);
      digitalWrite(Relais[1], LOW);
      webServer.send(200, "text/plain", "Toggled Solenoid Valve");
  });

  webServer.begin();

  Serial.println("Web Server started on Core: ");
  Serial.println(xPortGetCoreID());
}