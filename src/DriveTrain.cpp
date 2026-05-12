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
    // Store commands locally first to ensure atomic calculation
    float targetLinear = constrain(linearMmS, -MaxLinearSpeedMmS, MaxLinearSpeedMmS);
    float targetAngular = constrain(angularRadS, -MaxAngularSpeedRadS, MaxAngularSpeedRadS);

    currentCommand = {targetLinear, targetAngular};

    WheelSpeeds speeds = calculateWheelSpeeds(currentCommand);
    Serial.printf("[Drive] Command: Linear=%.1f mm/s, Angular=%.2f rad/s | Calculated Speeds: Left=%.1f mm/s, Right=%.1f mm/s\n",
                  targetLinear, targetAngular, speeds.leftMmS, speeds.rightMmS);
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

    // Safety: If rotation is requested but PWM ends up too low, 
    // we ensure we hit the minimum threshold if there's any significant speed requested.
    if (fabs(leftMmS) > 0.5f && lpwm == 0) lpwm = 151;
    if (fabs(rightMmS) > 0.5f && rpwm == 0) rpwm = 151;

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
