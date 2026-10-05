#include <Arduino.h>
#include <MiniPID.h>

const int MotorPin = 10;
const int PhotoPin = A5;

const int PWM_FREQ = 20000;
const int PWM_BITS = 8;
const int CHANNEL1 = 0;

// PID Base
MiniPID pid = MiniPID(1, 0.2, 0);
 
// State machine for determine ticks in the optical encoder
enum photoState {YesLight, NoLight, pCount};
photoState photoMode = NoLight;
int pastPhotoMode = 100;

int targetRPM = 90;

long lastTickTime = 0;
double currentRPM = 0;

long lastPIDTime = 0;
int pidInterval = 50; 

void setup() {
    Serial.begin(9600);
    delay(2000);
    
    ledcSetup(CHANNEL1, PWM_FREQ, PWM_BITS);
    ledcAttachPin(MotorPin, CHANNEL1);
    
    // PID Settings
    ////////////////
    // PID output limit
    pid.setOutputLimits(0, 255);
    // Prevent integral windup by capping the max I term
    pid.setMaxIOutput(150); 
    
    lastTickTime = millis();
    lastPIDTime = millis();
}

void loop() {
    int sensorValue = analogRead(PhotoPin);
    
    if (sensorValue >= 3000) {
        photoMode = YesLight;
    } else if (sensorValue < 1000) {
        photoMode = NoLight;
    }
    
    if (photoMode != pastPhotoMode) {
        long currentTime = millis();
        long timeBetweenTicks = currentTime - lastTickTime;
        
        if (timeBetweenTicks > 0) {
            double instantRPM = (1.0 / 10.0) * (60000.0 / timeBetweenTicks);
            
            // EMA Filter: Smoothens out the rpm readings
            currentRPM = (currentRPM * 0.7) + (instantRPM * 0.3);
        }
        
        lastTickTime = currentTime;
        pastPhotoMode = photoMode;
    }

    if (millis() - lastTickTime > 500) {
        currentRPM = 0;
    }

    // PID, Motor, and Printing Execution/Updating
    if (millis() - lastPIDTime >= pidInterval) {
        double output = pid.getOutput(currentRPM, targetRPM);
        
        // Add Base Power Offset outside the PID math because there is static friction to overcome
        // this way PID only focuses on generating output for everything after initial static friction
        double finalPWM = 0;
        if (targetRPM > 0) {
            finalPWM = output + 110; 
        }
        
        finalPWM = constrain(finalPWM, 0, 255);
        ledcWrite(CHANNEL1, finalPWM);
        
        Serial.print("RPM: ");
        Serial.print(currentRPM);
        Serial.print(" | PID Output: ");
        Serial.println(finalPWM);
        
        lastPIDTime = millis();
    }
    
    // At least 1ms delay needed to no crash esp32
    delay(1);
}