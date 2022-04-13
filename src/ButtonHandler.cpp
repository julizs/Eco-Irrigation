#include <ButtonHandler.h>

// Definition of static Variables
uint16_t ButtonHandler::lastDebounceTime = 0;
uint8_t ButtonHandler::debounceDelay = 50;
uint8_t ButtonHandler::buttonState = HIGH;
uint8_t ButtonHandler::lastButtonState = HIGH;


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