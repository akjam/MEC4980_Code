#include <Arduino.h>
#include <AccelStepper.h>

int pinStep = 9;
int pinDir = 10;

AccelStepper onlyAxis(1, pinStep, pinDir);

void setup() {
  pinMode(pinStep, OUTPUT);
  pinMode(pinDir, OUTPUT);

  onlyAxis.setMaxSpeed(2500);
  //onlyAxis.setSpeed(1200);
  onlyAxis.setAcceleration(700);

  //onlyAxis.moveTo(10000);

  // Repeat 5 times
  for (int i = 0; i < 5; i++) {
    onlyAxis.move(10000);       // Add 10,000 steps to current position
    onlyAxis.runToPosition();   // Accelerate, move, and decelerate to a stop before moving on
    //delay(500);                 // Optional: pause half a second between each run so you can see it stop
  }
}

void loop () {
  //onlyAxis.runSpeed();
  //onlyAxis.run();
}