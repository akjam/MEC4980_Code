#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

int joystickX = A1;
int joystickY = A0;
int pinRx = 38;
int pinTx = 39;

long rateToPc = 115200;
long rateToBoard = 9600;

int centerX = 0;
int centerY = 0;
int buffer = 80;
int xValue = 0;
int yValue = 0;
int xDirection = 1;
int yDirection = 1;

unsigned long lastSend = 0;
int sendInterval = 30;

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int fieldX = 5;
int fieldY = 5;
int fieldWidth = 230;
int fieldHeight = 125;

float pinX = 120;
float pinY = 67;

int oldX = 120;
int oldY = 67;
float maxSpeed = 85.0;

void calibrateJoystick() {
  long sumX = 0;
  long sumY = 0;
  Serial.println("Calibrating Joystick...");
  Serial.println("Keep joystick centered");
  delay(1000);

  for (int i = 0; i < 100; i++) {
    sumX += analogRead(joystickX);
    sumY += analogRead(joystickY);
    delay(5);
  }
  centerX = sumX / 100;
  centerY = sumY / 100;
  Serial.println("Calibration Complete!");
}

int joystickValue(int raw, int center) {

  int difference = raw - center;

  if (abs(difference) <= buffer) {
    return 0;
  }
  int value = 0;
  if (difference > 0) {
    int range = 4095 - center - buffer;
    if (range <= 0) {
      return 0;
    }
    value = (difference - buffer) * 100 / range;
  } else {
    int range = center - buffer;
    if (range <= 0) {
      return 0;
    }
    value = -((-difference - buffer) * 100 / range);
  }

  return constrain(value, -100, 100);
}

void drawScreen() {

  display.fillScreen(ST77XX_BLACK);
  display.drawRect(fieldX, fieldY, fieldWidth, fieldHeight, ST77XX_WHITE);
  display.fillCircle((int)pinX, (int)pinY, 5, ST77XX_YELLOW);
}

void updateScreen(float dt) {

  display.fillCircle( oldX, oldY, 5, ST77XX_BLACK);
  pinX += (xValue / 100.0f) * maxSpeed * dt;
  pinY -= (yValue / 100.0f) * maxSpeed * dt;
  pinX = constrain( pinX, (float)(fieldX + 8), (float)(fieldX + fieldWidth - 9));
  pinY = constrain(pinY, (float)(fieldY + 8), (float)(fieldY + fieldHeight - 9));

  oldX = roundf(pinX);
  oldY = roundf(pinY);

  display.fillCircle(oldX, oldY, 5, ST77XX_YELLOW);
}

void setup() {

  Serial.begin(rateToPc);
  Serial1.setPins(pinRx, pinTx);
  Serial1.begin(rateToBoard);

  analogReadResolution(12);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);

  delay(100);

  display.init(135, 240);
  display.setRotation(3);

  calibrateJoystick();

  drawScreen();
  lastSend = millis();

  Serial.println("Controller Ready!");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSend >= sendInterval) {

    float dt = (currentTime - lastSend) / 1000.0f;
    lastSend = currentTime;

    int rawX = analogRead(joystickX);
    int rawY = analogRead(joystickY);

    xValue = joystickValue(rawX, centerX);
    yValue = joystickValue(rawY, centerY);
    xValue *= xDirection;
    yValue *= yDirection;

    Serial1.print("J,");
    Serial1.print(xValue);
    Serial1.print(",");
    Serial1.println(yValue);

    Serial.print("X: ");
    Serial.print(xValue);

    Serial.print(" | Y: ");
    Serial.println(yValue);

    updateScreen(dt);
  }
}
