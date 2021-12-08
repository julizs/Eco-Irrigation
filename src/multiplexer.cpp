#include "multiplexer.h"

Multiplexer::Multiplexer()
{

}

void Multiplexer::setup()
{
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(analogPin, INPUT);
  
  // Like this or via Esp Port Manipulation
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
}

int Multiplexer::readChannel(byte channel)
{
     int controlPin[] = {S0, S1, S2, S3}; 
     int muxChannel[16][4]={ 
     {0,0,0,0}, // chann 0
     {1,0,0,0}, // chann 1
     {0,1,0,0}, // ...
     {1,1,0,0}, 
     {0,0,1,0}, 
     {1,0,1,0}, 
     {0,1,1,0}, 
     {1,1,1,0}, 
     {0,0,0,1}, 
     {1,0,0,1}, 
     {0,1,0,1}, 
     {1,1,0,1}, 
     {0,0,1,1}, 
     {1,0,1,1}, 
     {0,1,1,1}, 
     {1,1,1,1}};
      
     // digitalWrite to each controlPin (S0-S3) HIGH/LOW Value
     // according to 2D Array to select Channel, then read analog
     for(int i = 0; i < 4; i ++)
     { 
       digitalWrite(controlPin[i], muxChannel[channel][i]); 
     }

     int val = analogRead(analogPin);  
     return val;
}