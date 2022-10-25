#include <FlowMeter.h>

/*
https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
https://electropeak.com/learn/interfacing-yf-s201c-transparent-water-liquid-flow-sensor-with-arduino/
https://techtutorialsx.com/2017/09/30/esp32-arduino-external-interrupts/
https://circuitdigest.com/microcontroller-projects/esp32-interrupt
*/

FlowMeter::FlowMeter(uint8_t pinNum)
{
  this->pinNum = pinNum;

  measurement.pulses = 0;
  measurement.amountMl = 0;
  measurement.flowLperMin = 0;
  measurement.flowLperHour = 0;
  measurement.flowMlperSec = 0;

  // setup();
}

/*
Do in main instead of here, Class method has hidden this param
*/
void FlowMeter::setup()
{
    // attachInterrupt(button2Pin, pulse, RISING);
}

/*
Called frequently called in Pump:LOOP
(Flow could change, make Point every Invtervall, e.g. 1.0s)
Same as INA219
*/
FlowData FlowMeter::measureFlow()
{ 
  measurement.flowLperMin = (measurement.pulses / 7.5); // Q(flowRate L/Min) = pulses / 7.5 (see Datasheet)
  measurement.flowLperHour = measurement.flowLperMin * 60;
  measurement.flowMlperSec = (measurement.flowLperMin / 60) * 1000;

  return measurement;
}

/*
Make Point every Measure Intervall (e.g. 1s), write to Buffer
More detailed Illustr. of waterFlow in Time
Do same for INA219
*/
void FlowMeter::writePoint()
{
  // Point p3("Water Flow");
  p3.clearTags();
  p3.clearFields();

  // p3.addTag("pump", instr.pumpModel);
  // p3.addTag("solenoidValve", solenoidValve);
  p3.addField("flowLperHour", measurement.flowLperHour);
  p3.addField("flowMlperSec", measurement.flowMlperSec);

  char message[64];
  snprintf(message, 64, "Flowmeter: Waterflow (L/hour): %.2f, Waterflow (Ml/sec): %.2f", measurement.flowLperHour, measurement.flowMlperSec);
  Serial.println(message);

  InfluxHelper::writeDataPoint(p3);
}

/*
Called only once in Pump::DONE
Calc Ml from total pulses, reset counter
Write this date only once into Point p2
*/
void FlowMeter::measureVolume()
{
    measurement.amountMl = measurement.pulses * 2.25; // 1 Pulse = 2.25 Ml (see Datasheet)
    measurement.pulses = 0;

    p2.addField("distributedWater_flowMeter", measurement.amountMl);

    Serial.print("FlowMeter Amount (Ml): ");
    Serial.println(measurement.amountMl);
}

/*
ISR (Interrupt Service Routine)
*/
void FlowMeter::pulse ()
{
  measurement.pulses++;
}