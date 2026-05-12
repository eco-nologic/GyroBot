#include "DCMotor.h"
#include <Arduino.h>

// Global ISR handlers (max 2 motors)
static DCMotor* g_motorLeft = nullptr;
static DCMotor* g_motorRight = nullptr;

void IRAM_ATTR isr_leftMotor() {
    if (g_motorLeft) g_motorLeft->processEncoderChange();
}

void IRAM_ATTR isr_rightMotor() {
    if (g_motorRight) g_motorRight->processEncoderChange();
}

DCMotor::DCMotor(int en, int in1, int in2, int encA, int encB)
    : enablePin(en), in1Pin(in1), in2Pin(in2), encoderAPin(encA), encoderBPin(encB) {
    // Store global references for ISR
    if (en == PinMotorLeftEn) {
        g_motorLeft = this;
    } else if (en == PinMotorRightEn) {
        g_motorRight = this;
    }
}

void DCMotor::begin() {
    pinMode(enablePin, OUTPUT);
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);

    pinMode(encoderAPin, INPUT_PULLUP);
    pinMode(encoderBPin, INPUT_PULLUP);

    digitalWrite(enablePin, 0);
    digitalWrite(in1Pin, 0);
    digitalWrite(in2Pin, 0);

    // Attach encoder ISR to channel A
    if (enablePin == PinMotorLeftEn) {
        attachInterrupt(digitalPinToInterrupt(encoderAPin), isr_leftMotor, CHANGE);
    } else if (enablePin == PinMotorRightEn) {
        attachInterrupt(digitalPinToInterrupt(encoderAPin), isr_rightMotor, CHANGE);
    }

    lastHealthCheck = millis();
}

void DCMotor::setPwm(int pwm) {
    // Clamp to 0-255 and apply deadband (motor won't move below ~151 PWM)
    pwm = constrain(pwm, 0, 255);
    if (pwm > 0 && pwm < 151) pwm = 151;

    if (pwm > 0) {
        Serial.printf("[Motor] PWM output: %d on Pin %d\n", pwm, enablePin);
    }

    analogWrite(enablePin, pwm);
}

void DCMotor::setDirection(bool forward) {
    if (forward) {
        digitalWrite(in1Pin, HIGH);
        digitalWrite(in2Pin, LOW);
    } else {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, HIGH);
    }
}

long DCMotor::getEncoderCount() {
    return encoderCount;
}

void DCMotor::resetEncoder() {
    encoderCount = 0;
    lastReportedCount = 0;
}

bool DCMotor::isHealthy() {
    // Check if encoder is responding (compare with last reported count)
    unsigned long now = millis();
    if (now - lastHealthCheck > 1000) {
        lastHealthCheck = now;
        motorHealthy = (encoderCount != lastReportedCount);
        lastReportedCount = encoderCount;
    }
    return motorHealthy;
}

void DCMotor::processEncoderChange() {
    int a = digitalRead(encoderAPin);
    int b = digitalRead(encoderBPin);

    // Quadrature decoder (detect direction from A/B phase)
    if (a != lastEncoderA) {
        if (a == b) {
            encoderCount++; // Forward
        } else {
            encoderCount--; // Backward
        }
        lastEncoderA = a;
    }
}
