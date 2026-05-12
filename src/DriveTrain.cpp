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

    if (leftMotor) {
        leftMotor->setDirection(leftMmS >= 0);
        leftMotor->setPwm(speedToPwm(fabs(leftMmS)));
    }

    if (rightMotor) {
        rightMotor->setDirection(rightMmS >= 0);
        rightMotor->setPwm(speedToPwm(fabs(rightMmS)));
    }
}

int DriveTrain::speedToPwm(float speedMmS) {
    // Convert speed (mm/s) to PWM (0-255)
    if (speedMmS < 5.0f) return 0;
    int pwm = (int)((speedMmS / MaxLinearSpeedMmS) * 255.0f);
    return constrain(pwm, 0, 255);
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
