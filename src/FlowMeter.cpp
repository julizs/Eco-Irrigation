#include <FlowMeter.h>

FlowMeter::FlowMeter(uint8_t pinNum)
{
  this->pinNum = pinNum;

  pulses = 0;
  measurement.amountMl = 0;
  measurement.flowLperMin = 0;
  measurement.flowLperHour = 0;
  measurement.flowMlperSec = 0;

  // setup();
}

/*
Do in main instead of here, Class method hides "this" param
*/
void FlowMeter::setup()
{
    // attachInterrupt(button2Pin, pulse, RISING);
}

/*
Called frequently in Pump:LOOP (same as INA219)
(Point every Invtervall to see changes)
Q(flowRate L/Min) = pulses / 7.5 (see Datasheet)
*/
FlowData FlowMeter::measureFlow()
{ 
  measurement.flowLperMin = (pulses / 7.5);
  measurement.flowLperHour = measurement.flowLperMin * 60;
  measurement.flowMlperSec = (measurement.flowLperMin / 60) * 1000;

  return measurement;
}

/*
Called only once in Pump::DONE
Calc Ml from total pulses, reset counter
Write this date only once into Point p2
*/
uint16_t FlowMeter::measureAmount()
{
    measurement.amountMl = pulses * 2.25; // 1 Pulse = 2.25 Ml (see Datasheet)
    pulses = 0;

    Serial.print("FlowMeter Amount (Ml): ");
    Serial.println(measurement.amountMl);

    /*
    p2.clearTags();
    p2.clearFields();
    p2.addField("distributedWater_flowMeter", measurement.amountMl);
    InfluxHelper::writeDataPoint(p2);
    */

    return measurement.amountMl;
}

void FlowMeter::writePoint()
{
  // FlowData measurement = measureFlow();

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
ISR (Interrupt Service Routine)
*/
void FlowMeter::pulse ()
{
  pulses++;
}