#include <Arduino.h>
#include <AccelStepper.h>
#include <math.h>

// =========================================================================
// QUICK TOGGLES
// =========================================================================
// Set to true when testing upside down on the floor.
// Change to false when mounted under the Klask table.
bool isUpsideDown = false;

// Set to true to enable the circular no-go zone around the goal.
bool enableGoalNoGoZone = true;

// =========================================================================
// 1. DIGITAL HARDSTOP, FLAT CEILING & GOAL NO-GO ZONE PARAMETERS
// =========================================================================
float collinearCosThreshold = -0.94f; 
float armReachMargin        = 0.08f;  // Inches backed off from physical lockup/fold
float maxJointAngleCos      = 0.995f; // Mathematical safety clamp for acos()

// Rectangular board boundaries (in inches).
// Setting fieldMaxY = 2.00f makes the ceiling a flat horizontal line from wall
// to wall so the mechanism never touches the curved singularity arch.
float fieldMinX = -4.4f;
float fieldMaxX =  4.4f;
float fieldMinY = -3.125f;
float fieldMaxY =  1.90f;  // Flat straight-line ceiling across the board

// Goal No-Go Zone (4" diameter circle touching starting point (0.00, -0.50) at top)
float goalCenterX =  0.00f;
float goalCenterY = -2.50f;
float goalRadius  =  2.00f;

// =========================================================================
// 2. MOTION & HIGH-FREQUENCY INTERPOLATION PARAMETERS
// =========================================================================
float normalSpeed   = 12000.0f; // Max step speed
float normalAccel   = 18000.0f; // Acceleration ramp
float moveStep      = 0.3f;    // Total inches moved per 30ms joystick window

// Slices each 30ms joystick packet into 3ms micro-updates (10x smoother)
const unsigned long ikUpdateIntervalMs = 3;

// =========================================================================
// 3. HARDWARE & LINKAGE SETUP
// =========================================================================
int pinRx = 38;
int pinTx = 39;
int pinStepLeft = 9;
int pinDirLeft = 10;
int pinStepRight = 6;
int pinDirRight = 5;

long rateToPc = 115200;
long rateToBoard = 9600;

float link1 = 5.50f;
float link2 = 7.65f;
float leftMotorX = -7.18f;
float rightMotorX = 7.18f;

float stepsPerDegree = 3200.0f / 360.0f; // 1/16 microstepping

float currentX = 0.00f;
float currentY = -0.50f;
float homeAngleLeft = 68.96f;
float homeAngleRight = 111.04f;

int xValue = 0;
int yValue = 0;
unsigned long lastIkUpdateTime = 0;
unsigned long lastSerialPacketTime = 0;

String boardbuffer = "";

AccelStepper stepperLeft(AccelStepper::DRIVER, pinStepLeft, pinDirLeft);
AccelStepper stepperRight(AccelStepper::DRIVER, pinStepRight, pinDirRight);

// Returns true if (x, y) is inside the prohibited goal area
bool IsInsideGoalZone(float x, float y) {
  if (!enableGoalNoGoZone) return false;

  float dx = x - goalCenterX;
  float dy = y - goalCenterY;
  float effectiveRadius = goalRadius - 0.002f; // Leaves (0.00, -0.50) valid at startup

  // Block the circular goal area
  if ((dx * dx + dy * dy) < (effectiveRadius * effectiveRadius)) {
    return true;
  }

  // Block any pocket directly underneath the circle's equator down to the back wall
  if (y < goalCenterY && fabs(dx) < effectiveRadius) {
    return true;
  }

  return false;
}

// Inverse Kinematics with Digital Hardstop & Goal No-Go Protection
bool CalculateIK(float x, float y, float &outLeftSteps, float &outRightSteps) {
  if (IsInsideGoalZone(x, y)) {
    return false;
  }

  // Left Arm
  float dxl = x - leftMotorX;
  float dyl = y;
  float cl = sqrtf(dxl * dxl + dyl * dyl);

  if (cl > (link1 + link2 - armReachMargin) || cl < (fabs(link2 - link1) + armReachMargin)) {
    return false;
  }

  float cosBetaL = (link1 * link1 + cl * cl - link2 * link2) / (2.0f * link1 * cl);
  if (cosBetaL < -maxJointAngleCos || cosBetaL > maxJointAngleCos) return false;
  float betaL = acosf(cosBetaL);
  float alphaL = atan2f(dyl, dxl);
  float thetaL = alphaL + betaL;

  // Right Arm
  float dxr = x - rightMotorX;
  float dyr = y;
  float cr = sqrtf(dxr * dxr + dyr * dyr);

  if (cr > (link1 + link2 - armReachMargin) || cr < (fabs(link2 - link1) + armReachMargin)) {
    return false;
  }

  float cosBetaR = (link1 * link1 + cr * cr - link2 * link2) / (2.0f * link1 * cr);
  if (cosBetaR < -maxJointAngleCos || cosBetaR > maxJointAngleCos) return false;
  float betaR = acosf(cosBetaR);
  float alphaR = atan2f(dyr, dxr);
  if (alphaR < 0.0f) {
    alphaR += 2.0f * PI;
  }
  float thetaR = alphaR - betaR;

  // Collinear L2 Singularity Safety Check
  float leftElbowX = leftMotorX + link1 * cosf(thetaL);
  float leftElbowY = link1 * sinf(thetaL);
  float rightElbowX = rightMotorX + link1 * cosf(thetaR);
  float rightElbowY = link1 * sinf(thetaR);

  float vLx = leftElbowX - x;
  float vLy = leftElbowY - y;
  float vRx = rightElbowX - x;
  float vRy = rightElbowY - y;

  float dot = vLx * vRx + vLy * vRy;
  float cosGamma = dot / (link2 * link2);

  if (cosGamma <= collinearCosThreshold) {
    return false;
  }

  float leftDeg = thetaL * (180.0f / PI);
  float rightDeg = thetaR * (180.0f / PI);

  outLeftSteps = roundf((leftDeg - homeAngleLeft) * stepsPerDegree);
  outRightSteps = roundf((rightDeg - homeAngleRight) * stepsPerDegree);
  return true;
}

// Simple direct move within the rectangular boundaries
void UpdateKinematicsStep() {
  // Stop moving if joystick board has not sent data in over 100ms
  if (millis() - lastSerialPacketTime > 100) {
    xValue = 0;
    yValue = 0;
  }

  if (xValue == 0 && yValue == 0) {
    return;
  }

  // Scale moveStep down to match the 3ms update rate (1/10th per tick)
  float subStepRatio = (float)ikUpdateIntervalMs / 30.0f;
  float activeStep = moveStep * subStepRatio;

  // Clamp directly to the flat-ceiling rectangular box
  float nextX = constrain(currentX + (xValue / 100.0f) * activeStep, fieldMinX, fieldMaxX);
  float nextY = constrain(currentY + (yValue / 100.0f) * activeStep, fieldMinY, fieldMaxY);

  float targetLeftSteps = 0.0f;
  float targetRightSteps = 0.0f;

  // 1. Try combined X and Y movement
  if (CalculateIK(nextX, nextY, targetLeftSteps, targetRightSteps)) {
    currentX = nextX;
    currentY = nextY;
    stepperLeft.moveTo(targetLeftSteps);
    stepperRight.moveTo(targetRightSteps);
  }
  // 2. Try X-only movement (if Y is blocked by the goal zone)
  else if (CalculateIK(nextX, currentY, targetLeftSteps, targetRightSteps)) {
    currentX = nextX;
    stepperLeft.moveTo(targetLeftSteps);
    stepperRight.moveTo(targetRightSteps);
  }
  // 3. Try Y-only movement (if X is blocked by the goal zone)
  else if (CalculateIK(currentX, nextY, targetLeftSteps, targetRightSteps)) {
    currentY = nextY;
    stepperLeft.moveTo(targetLeftSteps);
    stepperRight.moveTo(targetRightSteps);
  }
}

void setup() {
  Serial.begin(rateToPc);
  Serial1.setPins(pinRx, pinTx);
  Serial1.begin(rateToBoard);

  stepperLeft.setMaxSpeed(normalSpeed);
  stepperLeft.setAcceleration(normalAccel);
  stepperRight.setMaxSpeed(normalSpeed);
  stepperRight.setAcceleration(normalAccel);
}

void RunSecondBoardLoop() {
  // 1. Read incoming joystick values whenever a packet finishes arriving
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      boardbuffer.trim();
      if (boardbuffer.length() > 0) {
        int comma1 = boardbuffer.indexOf(',');
        int comma2 = boardbuffer.lastIndexOf(',');
        xValue = boardbuffer.substring(comma1 + 1, comma2).toInt();
        yValue = boardbuffer.substring(comma2 + 1).toInt();

        lastSerialPacketTime = millis();

        if (isUpsideDown) {
          xValue = -xValue;
        }
      }
      boardbuffer = "";
    } else if (c != '\r') {
      boardbuffer += c;
    }
  }

  // 2. Run the kinematics calculation every 3ms
  if (millis() - lastIkUpdateTime >= ikUpdateIntervalMs) {
    lastIkUpdateTime = millis();
    UpdateKinematicsStep();
  }

  // 3. Continuously pulse the stepper motors
  stepperLeft.run();
  stepperRight.run();
}

void loop() {
  RunSecondBoardLoop();
}