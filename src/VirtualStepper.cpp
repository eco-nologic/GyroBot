#include "VirtualStepper.h"
#include <Arduino.h>

VirtualStepper::VirtualStepper() : speedMmS(0), direction(true), encoderCount(0) {}

void VirtualStepper::begin() {
    // No hardware initialization needed
}

void VirtualStepper::setPwm(int pwm) {
    // Convert PWM (0-255) to speed (0-MaxLinearSpeedMmS)
    speedMmS = (pwm / 255.0f) * MaxLinearSpeedMmS;
}

void VirtualStepper::setDirection(bool forward) {
    direction = forward;
}

long VirtualStepper::getEncoderCount() {
    return encoderCount;
}

void VirtualStepper::resetEncoder() {
    encoderCount = 0;
}

void VirtualStepper::update(float deltaTime) {
    // Simulate encoder ticks based on speed and time elapsed
    float distanceMm = speedMmS * deltaTime;
    float stepsPerMm = StepsPerRev / (PI * WheelDiameterMm);
    long stepsDelta = (long)(distanceMm * stepsPerMm);

    if (direction) {
        encoderCount += stepsDelta;
    } else {
        encoderCount -= stepsDelta;
    }
}
