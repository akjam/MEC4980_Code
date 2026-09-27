#include <Arduino.h>
#include <Adafruit_BNO08x.h>
#include <math.h>
#include <AceButton.h>
#include <Adafruit_ST7789.h>
using namespace ace_button;

void setReports();
#define BNO08X_RESET -1
int pinD1 = 1;
int pinD0 = 0;
int pinD2 = 2;
AceButton button0(pinD0);
AceButton button1(pinD1);
AceButton button2(pinD2);

/*enum AxisMode {
  MODE_BOTH,
  MODE_X,
  MODE_Y,
  MODE_COUNT
};
AxisMode curMode = MODE_BOTH;*/

enum screenMode {
  stepsScreen,
  distanceScreen,
  strideScreen,
  rawScreen,
  screenCount
};
screenMode curScreen = stepsScreen;

/*void ChangeMode(AceButton * button1, uint8_t eventType, uint8_t buttonState) {

  Serial.print(F("handleEvent(): eventType: "));
  Serial.print(AceButton::eventName(eventType));
  Serial.print(F("; buttonState: "));
  Serial.println(buttonState);
  if (eventType == (uint8_t)AceButton::kEventDoubleClicked) {
    curMode = (AxisMode)((curMode + 1) % AxisMode::MODE_COUNT);
  }
}*/

float strideLength = 3; //units of ft
void handleEvent(AceButton * button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      if (button->getPin() == pinD1) {
        curScreen = (screenMode)((curScreen + 1) % screenMode::screenCount);
      } else if (curScreen == strideScreen) {
        if (button->getPin() == pinD2) {
          strideLength += 0.1;
        } else if (button->getPin() == pinD0) {
          strideLength -= 0.1;
        }
      }
      break;
  }
} 

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

void setup(void) {
  Serial.begin(115200);
  //while (!Serial)
    //delay(10); // will pause Zero, Leonardo, etc until serial console opens

  pinMode(pinD0, INPUT_PULLUP);
  pinMode(pinD1, INPUT_PULLDOWN);
  pinMode(pinD2, INPUT_PULLDOWN);
  button0.init(pinD0, LOW);
  button1.init(pinD1, HIGH);
  button2.init(pinD2, HIGH);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 0);
  display.init(135, 240);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  digitalWrite(TFT_BACKLITE, 1);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  //buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  
  Serial.println("Adafruit BNO08x test!");

  // Try to initialize!
  if (!bno08x.begin_I2C()) {
    // if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte
    // UART buffer! if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("BNO08x Found!");
}

void loop() {
  button1.check(); //detects if button was pressed
  button0.check(); //detects if button was pressed
  button2.check(); //detects if button was pressed
  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }

  static int stepCount = 0;
  static float x = 0;
  static float y = 0;
  static float z = 0;


while (bno08x.getSensorEvent(&sensorValue)) {
    switch (sensorValue.sensorId) {
      case SH2_STEP_COUNTER:
        stepCount = sensorValue.un.stepCounter.steps;
        break;
      case SH2_ACCELEROMETER:
        x = sensorValue.un.accelerometer.x;
        y = sensorValue.un.accelerometer.y;
        z = sensorValue.un.accelerometer.z;
        break;
    }
  }
 /*if (!bno08x.getSensorEvent(&sensorValue)) {
  return;
 }

  int stepCount = sensorValue.un.stepCounter.steps;
  double x = sensorValue.un.accelerometer.x;
  double y = sensorValue.un.accelerometer.y;
  double z = sensorValue.un.accelerometer.z;*/

  double distanceTraveled = stepCount * strideLength; // units of feet
  
  if (strideLength > 6) {
    strideLength = 1;
  } else if (strideLength < 0) {
    strideLength = 6;
  }

  canvas.fillScreen(ST77XX_BLACK);
  canvas.setCursor(0,0);
  canvas.setTextSize(2);
  
  if (curScreen == stepsScreen){
    canvas.print("Step Count: ");
    canvas.print(stepCount);
  }
  
  if (curScreen == distanceScreen){
    canvas.println("Distance ");
    canvas.print("Traveled: ");
    canvas.print(distanceTraveled);
    canvas.print(" ft ");
  }
  
  if (curScreen == strideScreen){
    canvas.println("Stride ");
    canvas.print("Length: ");
    canvas.print(strideLength);
    canvas.print(" ft ");
    canvas.setCursor(0,50);
    canvas.println("Press D2 to increase");
    canvas.print("Press D0 to decrease");
  }
  
  if (curScreen == rawScreen){
    canvas.println("Raw Accelerometer");
    canvas.println("Readings: ");
    canvas.setCursor(0,50);
    canvas.print(" x: ");
    canvas.println(x);
    canvas.print(" y: ");
    canvas.println(y);
    canvas.print(" z: ");
    canvas.println(z);
  }
  
  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
    Serial.println("Could not enable step counter");
  }
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  } else {
    Serial.println("Set accelerometer report... success!");
  }
}