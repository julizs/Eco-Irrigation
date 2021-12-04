#include "multiplexer.h"

Multiplexer::Multiplexer()
{

}

void Multiplexer::setup()
{
  pinMode(Aa, OUTPUT);
  pinMode(Be, OUTPUT);
  pinMode(Ce, OUTPUT);
  pinMode(analogPin, INPUT);
  
  digitalWrite(Aa, LOW);
  digitalWrite(Be, LOW);
  digitalWrite(Ce, LOW);
}

int Multiplexer::measureAnalog(byte pin)
{
    return 0;
}