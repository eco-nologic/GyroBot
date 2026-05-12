#include "PoseEstimator.h"
#include <math.h>

PoseEstimator::PoseEstimator(DriveTrain* drive, Navigation* nav)
    : driveTrain(drive), navigation(nav) {
    robotPose = {0, 0, 0};
    ghostPose = {0, 0, 0};
}

void PoseEstimator::begin() {
    lastUpdateTime = millis();
    lastLeftEncoderCount = 0;
    lastRightEncoderCount = 0;
    lastHeading = 0.0f;
}

void PoseEstimator::update() {
    unsigned long now = millis();
    float dt = (now - lastUpdateTime) / 1000.0f;
    lastUpdateTime = now;

    if (dt < 0.001f) return; // Skip if too soon

    updateOdometry();
    fuseImuHeading();
}

void PoseEstimator::updateOdometry() {
    // Calculate distance traveled from encoder counts
    long leftCount = driveTrain->getLeftEncoderCount();
    long rightCount = driveTrain->getRightEncoderCount();

    long deltaLeft = leftCount - lastLeftEncoderCount;
    long deltaRight = rightCount - lastRightEncoderCount;

    lastLeftEncoderCount = leftCount;
    lastRightEncoderCount = rightCount;

    // Convert encoder steps to distance
    float distancePerStep = (PI * WheelDiameterMm) / StepsPerRev;
    float leftDistance = deltaLeft * distancePerStep;
    float rightDistance = deltaRight * distancePerStep;

    // Average distance and estimate heading change
    float avgDistance = (leftDistance + rightDistance) / 2.0f;
    float headingChange = (rightDistance - leftDistance) / WheelBaseMm;

    // Update ghost pose (pure odometry)
    ghostPose.theta += headingChange;
    ghostPose.x += avgDistance * cos(ghostPose.theta);
    ghostPose.y += avgDistance * sin(ghostPose.theta);

    // Update robot pose (will be fused with IMU)
    robotPose.theta += headingChange;
    robotPose.x += avgDistance * cos(robotPose.theta);
    robotPose.y += avgDistance * sin(robotPose.theta);
}

void PoseEstimator::fuseImuHeading() {
    if (!navigation) return;

    float imuHeading = navigation->getHeading();
    float odometryHeading = robotPose.theta;

    // Simple blending (more sophisticated Kalman filter could be used)
    robotPose.theta = ImuFusionAlpha * odometryHeading + (1.0f - ImuFusionAlpha) * imuHeading;
}

void PoseEstimator::resetPose(float x, float y, float theta) {
    robotPose = {x, y, theta};
    ghostPose = {x, y, theta};
    lastHeading = theta;
}

void PoseEstimator::setPose(float x, float y, float theta) {
    robotPose.x = x;
    robotPose.y = y;
    robotPose.theta = theta;
}
