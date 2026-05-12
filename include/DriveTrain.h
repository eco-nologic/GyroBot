#ifndef DRIVETRAIN_H
#define DRIVETRAIN_H

#include "IMotor.h"
#include "Config.h"
#include <Arduino.h>

struct WheelSpeeds {
    float leftMmS;   // Left wheel speed (mm/s)
    float rightMmS;  // Right wheel speed (mm/s)
};

struct MotionCommand {
    float linearVelocityMmS;   // Forward/backward speed (mm/s)
    float angularVelocityRadS; // Rotation speed (rad/s)
};

class DriveTrain {
private:
    IMotor* leftMotor;
    IMotor* rightMotor;

    WheelSpeeds currentWheelSpeeds;
    MotionCommand currentCommand;

    // Convert wheel speeds to PWM (0-255)
    int speedToPwm(float speedMmS);

    // Calculate individual wheel speeds from motion command
    WheelSpeeds calculateWheelSpeeds(const MotionCommand& cmd);

public:
    DriveTrain(IMotor* left, IMotor* right);
    ~DriveTrain() = default;

    void begin();

    // Set motion command (linear + angular velocity)
    void setMotion(float linearMmS, float angularRadS);

    // Stop all motors
    void stop();

    // Get current wheel speeds
    WheelSpeeds getWheelSpeeds() const { return currentWheelSpeeds; }

    // Get current motion command
    MotionCommand getMotionCommand() const { return currentCommand; }

    // Low-level wheel speed control
    void setWheelSpeeds(float leftMmS, float rightMmS);

    // Encoder odometry
    void resetOdometry();
    long getLeftEncoderCount() const;
    long getRightEncoderCount() const;
};

#endif // DRIVETRAIN_H
