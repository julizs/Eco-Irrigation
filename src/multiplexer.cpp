#include "multiplexer.h"

Multiplexer::Multiplexer()
{

}

void Multiplexer::setup()
{
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(analogPin, INPUT);
  
  digitalWrite(A, LOW);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
}

int Multiplexer::measureAnalog(byte pin)
{
    return 0;
}