#include <FlowMeter.h>

/*
https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
https://electropeak.com/learn/interfacing-yf-s201c-transparent-water-liquid-flow-sensor-with-arduino/
*/
FlowMeter::FlowMeter(uint8_t pinNum)
{
  this->pinNum = pinNum;
  measureIntervall = 1000;
  amountMl = 0;
}

void FlowMeter::setup()
{
    // Not possible, Class method has this param
    // attachInterrupt(2, pulse, RISING);
}

/*
Not neccessary if called in Pump::DONE instead of Pump:LOOP:
if(currentTime >= (lastTime + measureIntervall)) {...}
Count all pulses and calc Ml only once in Pump::DONE
*/
void FlowMeter::measureVolume()
{
    currentTime = millis();
    
    lastTime = currentTime; 
    flowLperM = (pulse_freq / 7.5); // Q(flowRate L/Min) = pulses / 7.5 (see Datasheet)
    flowLperH = flowLperM * 60;
    flowMlperSec = (flowLperM / 60) * 1000; // How many Ml in this measured Intervall
    amountMl = pulse_freq * 2.25; // In Ml
    pulse_freq = 0;
}

void FlowMeter::pulse () // Interrupt function
{
  pulse_freq++;
}