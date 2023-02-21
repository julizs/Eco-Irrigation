#include "Multiplexer.h"

int Multiplexer::controlPins[] = {S0, S1, S2, S3};
int Multiplexer::muxChannel[16][4] = {
      {0, 0, 0, 0}, // chann 0
      {1, 0, 0, 0}, // chann 1
      {0, 1, 0, 0}, // ...
      {1, 1, 0, 0},
      {0, 0, 1, 0},
      {1, 0, 1, 0},
      {0, 1, 1, 0},
      {1, 1, 1, 0},
      {0, 0, 0, 1},
      {1, 0, 0, 1},
      {0, 1, 0, 1},
      {1, 1, 0, 1},
      {0, 0, 1, 1},
      {1, 0, 1, 1},
      {0, 1, 1, 1},
      {1, 1, 1, 1}};

void Multiplexer::setupPins()
{
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  //pinMode(SIG, INPUT);
  
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
}

/*
Drive each controlPin (S0-S3) to HIGH/LOW according
to 2D Array to select Channel, then read analog
*/
uint16_t Multiplexer::readChannel(uint8_t channel)
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(controlPins[i], muxChannel[channel][i]);
  }

  delay(50);

  uint16_t val = analogRead(SIG);
  return val;
}