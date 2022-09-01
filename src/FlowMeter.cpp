#include <FlowMeter.h>

/*
https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
https://electropeak.com/learn/interfacing-yf-s201c-transparent-water-liquid-flow-sensor-with-arduino/
*/
FlowMeter::FlowMeter(uint8_t pinNum)
{
  this->pinNum = pinNum;
  pulse_freq = 0;
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
(Flow could change, make Point every 1.0s)
*/
void FlowMeter::measureFlow()
{ 
  flowLperMin = (pulse_freq / 7.5); // Q(flowRate L/Min) = pulses / 7.5 (see Datasheet)
  flowLperHour = flowLperMin * 60;
  flowMlperSec = (flowLperMin / 60) * 1000; // How many Ml in this Measureintervall
    
  Serial.println("FlowMeter Flow (L/H): ");
  Serial.println(flowLperHour);
  makePoint(flowLperHour, flowMlperSec); 
}

/*
Make Point every Measure Intervall (e.g. 1s), write to Buffer
More detailed Illustr. of waterFlow in Time
Do same for INA219
*/
void FlowMeter::makePoint(double flowLperHour, double flowMlperSec)
{
  Point p3("Flow");
  // p3.clearTags();
  // p3.clearFields();

  // p3.addTag("pump", instr.pumpModel);
  // p3.addTag("solenoidValve", solenoidValve);
  p3.addField("flowLperHour", flowLperHour);
  p3.addField("flowMlperSec", flowMlperSec);

  InfluxHelper::writeDataPoint(p3);
}

/*
Called only once in Pump::DONE, Count total Amount of pulses and calc Ml
Write this date only once into Point p2
*/
void FlowMeter::measureVolume()
{
    amountMl = pulse_freq * 2.25; // In Ml
    pulse_freq = 0;

    p2.addField("distributedWater_flowMeter", amountMl);
    Serial.println("FlowMeter Amount (Ml): ");
    Serial.println(amountMl);
}

void FlowMeter::pulse () // Interrupt function
{
  pulse_freq++;
}