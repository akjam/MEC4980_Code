#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <Arduino.h>
#include <time.h>
#include <Adafruit_ST7789.h>

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

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

void setup() {
  //Serial.begin(9600);
  //while (!Serial);
  //canvas.println(F("BME680 test"));
  //Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    canvas.println(F("Could not find a valid BME680 sensor, check wiring!"));
    //Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
    while (1);
  }

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);
  display.init(135, 240);
  display.setRotation(3);

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
    canvas.println(F("Failed to begin reading :("));
    //Serial.println(F("Failed to begin reading :("));
    return;
  }

  if (!bme.endReading()) {
    canvas.println(F("Failed to complete reading :("));
    //Serial.println(F("Failed to complete reading :("));
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
  canvas.fillScreen(ST77XX_BLACK);
  canvas.setCursor(0,0);
  canvas.setTextSize(2);
  if (opMode == Heating) {
    if (currentTempC < targetTempC) {
      digitalWrite(LED_YELLOW, HIGH);
      canvas.setTextColor(ST77XX_ORANGE);
      canvas.println("Heat is on now!");
      //Serial.println("Heat is on now!");
    } else {
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_BLUE, LOW);
    }
  } else if (opMode == Cooling) {
    if (currentTempC > targetTempC) {
      digitalWrite(LED_BLUE, HIGH);
      canvas.setTextColor(ST77XX_BLUE);
      canvas.println("AC is on now!");
      //Serial.println("AC is on now!");
    } else {
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_BLUE, LOW);
    }
  }

  canvas.setTextColor(ST77XX_WHITE);
  if (unitMode == F) {
    float currentTempF = (currentTempC * 9 / 5) + 32;
    float targetTempF = (targetTempC * 9 / 5) + 32;
    canvas.print(("Temperature: "));
    canvas.print(currentTempF);
    canvas.println("*F");
    canvas.print("Target: ");
    canvas.print(targetTempF);
    canvas.println("*F");
    /*Serial.print(("Temperature = "));
    Serial.print(currentTempF);
    Serial.println(" *F");
    Serial.print(" with target ");
    Serial.println(targetTempF);*/
  } else if (unitMode == C) {
    canvas.print(("Temperature: "));
    canvas.print(currentTempC);
    canvas.println("*C");
    canvas.print("Target: ");
    canvas.print(targetTempC);
    canvas.println("*C");
    /*Serial.print(("Temperature = "));
    Serial.print(currentTempC);
    Serial.println(" *C");
    Serial.print(" with target ");
    Serial.println(targetTempC);*/
  }

  canvas.print("Operation Mode: ");
  canvas.println((int)opMode);
  canvas.print("Menu #: ");
  canvas.println(menuMode);
  /*Serial.print(" operating in mode ");
  Serial.println((int)opMode);
  Serial.print(" in menu ");
  Serial.println(menuMode);*/
  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);

  delay(100);
}