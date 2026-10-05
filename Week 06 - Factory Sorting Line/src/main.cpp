#include <P1AM.h>

int modInput = 1;
int modOutput = 2;
int modAnalogIn = 3;

int pulseKey = 1;
int pinLB1 = 2;
int pinLB2 = 3;
int pinLBW = 4;
int pintLBR = 5;
int pintLBB = 6;

enum MachineStates {
  REST,
  SENSE,
  TRACKER,
  ACTUATE,
  COUNT
};

MachineState mState = REST;

void setup(){ // the setup routine runs once:

  Serial.begin(115200);  //initialize serial communication at 115200 bits per second 
  while (!P1.init()){  
  }
}

void TurnEverythingOff() {
  for (int i = 1; i < 6; i++) {
    P1.writeDiscrete(LOW, modOutput, i);
  }  
}


int channelTwo;
int color = 0;
void loop(){

  switch (mState)
  {
  case MachineStates::REST:
    TurnEverythingOff();
    if (!P1.readDiscrete(modInput, pinLB1)) {
      mState = MachineStates::SENSE;
    }
    break;
  case MachineStates::SENSE:
    P1.writeDiscrete(modOutput, 1);
    break;
  
  default:
    break;
  }

  Serial.print("pulse, 1, 2, W, R, B: ");
  for (int i = 1; i < 7; i++) {
	  channelTwo = P1.readDiscrete(modInput, i);	
    Serial.println(channelTwo);
    Serial.print(", ");
  }
  P1.readAnalog(modAnalogIn, 1);
  Serial.print(color);

}