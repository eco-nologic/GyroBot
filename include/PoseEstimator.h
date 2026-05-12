#ifndef POSEESTIMATOR_H
#define POSEESTIMATOR_H

#include <Arduino.h>
#include "Config.h"
#include "DriveTrain.h"
#include "Navigation.h"

struct Pose {
    float x;       // Position X (mm)
    float y;       // Position Y (mm)
    float theta;   // Heading (radians)
};

class PoseEstimator {
private:
    DriveTrain* driveTrain;
    Navigation* navigation;

    Pose robotPose = {0, 0, 0};
    Pose ghostPose = {0, 0, 0};  // Pure odometry (no IMU correction)

    long lastLeftEncoderCount = 0;
    long lastRightEncoderCount = 0;
    float lastHeading = 0.0f;

    unsigned long lastUpdateTime = 0;

    // Update odometry from wheel encoders
    void updateOdometry();

    // Fuse IMU heading into odometry
    void fuseImuHeading();

public:
    PoseEstimator(DriveTrain* drive, Navigation* nav);
    ~PoseEstimator() = default;

    void begin();
    void update();

    // Get robot pose (x, y, theta)
    Pose getPose() const { return robotPose; }

    // Get ghost pose (pure odometry, no IMU correction)
    Pose getGhostPose() const { return ghostPose; }

    // Reset pose to origin
    void resetPose(float x = 0, float y = 0, float theta = 0);

    // Set pose directly (for calibration)
    void setPose(float x, float y, float theta);

    // Get position only
    void getPosition(float& x, float& y) const {
        x = robotPose.x;
        y = robotPose.y;
    }

    // Get heading
    float getHeading() const { return robotPose.theta; }
};

#endif // POSEESTIMATOR_H
