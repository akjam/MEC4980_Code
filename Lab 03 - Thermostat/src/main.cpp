#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define LED_YELLOW 5
#define LED_BLUE 6

#define SEALEVELPRESSURE_HPA (1013.25)

enum hvacState {
  Heating, // 0
  Cooling, // 1
  hCount // 2
};

enum menuState {
  TemperatureMenu, //0
  OperationMenu, //1
  UnitMenu, //2
  mCount //3
};

enum unitState {
  C,  //0
  F,   //1
  uCount //2s
};

hvacState opMode = Heating;  // Initializing?
menuState menuMode = TemperatureMenu; // Initializing?
unitState unitMode = F; // Initializing?
float targetTempC = 24.;
volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;
long debounceTime = 50;
volatile bool changeButtonFlag = false;
volatile bool menuButtonFlag = false;

void IRAM_ATTR buttonToChangeThings() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    changeButtonFlag = true;
    prevChangeTime = now;      
  }
}

void IRAM_ATTR buttonToChangeMenu() {
  long now = millis();
  if (now > prevChangeTimeTwo + debounceTime) {
    menuButtonFlag = true;
    prevChangeTimeTwo = now;      
  }
}

Adafruit_BME680 bme(&Wire); // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToChangeThings, RISING);
  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangeMenu, RISING);

  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_2X);

  //bme.setHumidityOversampling(BME680_OS_2X);
  //bme.setPressureOversampling(BME680_OS_4X);
  //bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  //bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop() {
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }

  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }

  float currentTempC = bme.temperature; // Celcius by default

  if (menuButtonFlag) {  // if menuButtonFlag is true then this if loop activates
    menuButtonFlag = false;
    menuMode = (menuState)(((int)menuMode + 1) % (int)menuState::mCount);
  }

  if (changeButtonFlag) {
    if (menuMode == TemperatureMenu){
      targetTempC += 1.0;
      if (targetTempC > 30.0) {
        targetTempC = targetTempC -10.;
      }
    }
    if (menuMode == OperationMenu){
      opMode = (hvacState)(((int)opMode + 1) % (int)hvacState::hCount); // "%" returns just the remainder portion after dividing two terms"  
    }    
    if (menuMode == UnitMenu){
      unitMode = (unitState)(((int)unitMode + 1) % (int)unitState::uCount); // "%"
    }    
    changeButtonFlag = false; 
  }

  if (opMode == Heating) {
    if (currentTempC < targetTempC) {
      digitalWrite(LED_YELLOW, HIGH);
      Serial.println("Heat is on now!");
    } else {
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_BLUE, LOW);
    }
  } else if (opMode == Cooling) {
    if (currentTempC > targetTempC) {
      digitalWrite(LED_BLUE, HIGH);
      Serial.println("AC is on now!");
    } else {
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_BLUE, LOW);
    }
  }

  if (unitMode == F) {
    float currentTempF = (currentTempC * 9 / 5) + 32;
    float targetTempF = (targetTempC * 9 / 5) + 32;
    Serial.print(("Temperature = "));
    Serial.print(currentTempF);
    Serial.println(" *F");
    Serial.print(" with target ");
    Serial.println(targetTempF);
  } else if (unitMode == C) {
    Serial.print(("Temperature = "));
    Serial.print(currentTempC);
    Serial.println(" *C");
    Serial.print(" with target ");
    Serial.println(targetTempC);
  }

  Serial.print(" operating in mode ");
  Serial.println((int)opMode);
  Serial.print(" in menu ");
  Serial.println(menuMode);
  

  Serial.println();
  delay(100);
}