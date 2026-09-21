#include <Arduino.h>
#include <string.h>
#include <Ticker.h>

// put function declarations here:
int myFunction(int, int);
volatile String printSentence = "";
long prevSampleTime = 0;
long timeBetweenSamplesMs = 100;

void myFunction();

//Ticker newTimerFn(myFunction, timeBetweenSamplesMs, 0, MILLIS);

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  //delay(1000);

  int result = myFunction(2, 3);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);

  //newTimerFn.start();
}

void loop() {
  long currentTime = millis();
  //newTimerFn.update();
  if(strcmp(printSentence, ", ") {
    Serial.prinln(printSentence);
    printSentence = String("")
  }

  if (millis() > prevSampleTime + timeBetweenSamplesMs){
    Serial.println(printSentence);
    prevSampleTime = currentTime;
  }
}

// put function definitions here:
void myFunction() {
  Serial.print("D0, D1, D2, A0: ");
  printSentence += String(digitalRead(0)) + ", " + String(digitalRead(1)) + ", " + String(digitalRead(2));
  printSentence += ", " + String(analogRead(A0));  
}