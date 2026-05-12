#include "DriveTrain.h"
#include <math.h>

DriveTrain::DriveTrain(IMotor* left, IMotor* right)
    : leftMotor(left), rightMotor(right) {
    currentWheelSpeeds = {0, 0};
    currentCommand = {0, 0};
}

void DriveTrain::begin() {
    if (leftMotor) leftMotor->begin();
    if (rightMotor) rightMotor->begin();
}

void DriveTrain::setMotion(float linearMmS, float angularRadS) {
    currentCommand.linearVelocityMmS = constrain(linearMmS, -MaxLinearSpeedMmS, MaxLinearSpeedMmS);
    currentCommand.angularVelocityRadS = constrain(angularRadS, -MaxAngularSpeedRadS, MaxAngularSpeedRadS);

    WheelSpeeds speeds = calculateWheelSpeeds(currentCommand);
    setWheelSpeeds(speeds.leftMmS, speeds.rightMmS);
}

void DriveTrain::stop() {
    setMotion(0, 0);
}

WheelSpeeds DriveTrain::calculateWheelSpeeds(const MotionCommand& cmd) {
    // Differential drive kinematics
    // V_left  = V_linear - (ω × L/2)
    // V_right = V_linear + (ω × L/2)
    float wheelBaseRadius = WheelBaseMm / 2.0f;
    float angularContribution = cmd.angularVelocityRadS * wheelBaseRadius;

    WheelSpeeds speeds;
    speeds.leftMmS = cmd.linearVelocityMmS - angularContribution;
    speeds.rightMmS = cmd.linearVelocityMmS + angularContribution;

    return speeds;
}

void DriveTrain::setWheelSpeeds(float leftMmS, float rightMmS) {
    currentWheelSpeeds.leftMmS = leftMmS;
    currentWheelSpeeds.rightMmS = rightMmS;

    int lpwm = speedToPwm(fabs(leftMmS));
    int rpwm = speedToPwm(fabs(rightMmS));

    if (lpwm > 0 || rpwm > 0) {
        Serial.printf("[Drive] PWM L:%d, R:%d | Speeds L:%.1f, R:%.1f\n", lpwm, rpwm, leftMmS, rightMmS);
    }

    if (leftMotor) {
        leftMotor->setDirection(leftMmS >= 0);
        leftMotor->setPwm(lpwm);
    }

    if (rightMotor) {
        rightMotor->setDirection(rightMmS >= 0);
        rightMotor->setPwm(rpwm);
    }
}

int DriveTrain::speedToPwm(float speedMmS) {
    if (speedMmS < 1.0f) return 0;
    
    // Map speed range to usable PWM range (151 to 255)
    // This ensures even slow commanded speeds provide enough torque to move
    float ratio = speedMmS / MaxLinearSpeedMmS;
    int pwm = 151 + (int)(ratio * (255 - 151));
    return constrain(pwm, 151, 255);
}

void DriveTrain::resetOdometry() {
    if (leftMotor) leftMotor->resetEncoder();
    if (rightMotor) rightMotor->resetEncoder();
}

long DriveTrain::getLeftEncoderCount() const {
    return leftMotor ? leftMotor->getEncoderCount() : 0;
}

long DriveTrain::getRightEncoderCount() const {
    return rightMotor ? rightMotor->getEncoderCount() : 0;
}
