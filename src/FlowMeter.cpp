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
  pulses = 0;
  amountMl = 0;
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
void FlowMeter::measureFlow()
{ 
  flowLperMin = (pulses / 7.5); // Q(flowRate L/Min) = pulses / 7.5 (see Datasheet)
  flowLperHour = flowLperMin * 60;
  flowMlperSec = (flowLperMin / 60) * 1000;
  
  Serial.println("FlowMeter Flow (L/H): ");
  Serial.println(flowLperHour);
  writePoint(flowLperHour, flowMlperSec);
}

/*
Make Point every Measure Intervall (e.g. 1s), write to Buffer
More detailed Illustr. of waterFlow in Time
Do same for INA219
*/
void FlowMeter::writePoint(double flowLperHour, double flowMlperSec)
{
  // Point p3("Water Flow");
  p3.clearTags();
  p3.clearFields();

  // p3.addTag("pump", instr.pumpModel);
  // p3.addTag("solenoidValve", solenoidValve);
  p3.addField("flowLperHour", flowLperHour);
  p3.addField("flowMlperSec", flowMlperSec);

  InfluxHelper::writeDataPoint(p3);
}

/*
Called only once in Pump::DONE
Calc Ml from total pulses, reset counter
Write this date only once into Point p2
*/
void FlowMeter::measureVolume()
{
    amountMl = pulses * 2.25; // 1 Pulse = 2.25 Ml (see Datasheet)
    pulses = 0;

    p2.addField("distributedWater_flowMeter", amountMl);
    Serial.println("FlowMeter Amount (Ml): ");
    Serial.println(amountMl);
}

/*
ISR (Interrupt Service Routine)
*/
void FlowMeter::pulse ()
{
  pulses++;
}