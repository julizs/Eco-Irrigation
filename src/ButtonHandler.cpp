#include <ButtonHandler.h>

// Definition of static Variable
WebServer ButtonHandler::webServer(443);
unsigned long ButtonHandler::lastDebounceTime = 0;
unsigned short ButtonHandler::debounceDelay = 50;
int ButtonHandler::buttonState = HIGH;
int ButtonHandler::lastButtonState = HIGH;


void ButtonHandler::rootEndpoint()
{
  Serial.println("Web Server Clients handled on Core: ");
  Serial.println(xPortGetCoreID());

  webServer.send(200, "text/plain", "200 All is Ok.");
}

/*
Active Handling of WebButtons
*/
void ButtonHandler::startRestServer()
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
(e.g. Water Level needs to be checked first before Pump)
*/
void ButtonHandler::handleWebButtons()
{
  /*
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
  */
}

void ButtonHandler::handleHardwareButtons()
{
  int reading = digitalRead(button1Pin);

  // If Switch changed, due to Noise or Pressing:
  if (reading != lastButtonState)
  {
    // Reset Debouncing Timer
    lastDebounceTime = millis();
  }

  // Debouncing
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;

      if (buttonState == LOW)
      {
        // Serial.println("Pump Button pressed!");

        /*
        if (!wateringNeeded)
        {
          wateringNeeded = true;
        }
        else
        {
          wateringNeeded = false;
        }
        */

        /*
        if (fsm.currentState != 4) // otherwise ACTION state before PUMP_IDLE when pressed while Pumping
        {
          fsm.transitionTo(actionState);
        }
        */
      }
    }
  }
  lastButtonState = reading;
}