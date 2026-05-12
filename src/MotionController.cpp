#include "MotionController.h"
#include <math.h>
#include <climits>

MotionController::MotionController(DriveTrain* drive, PoseEstimator* pose)
    : driveTrain(drive), poseEstimator(pose) {
    pidLastTime = millis();
}

void MotionController::begin() {
    pidLastTime = millis();
}

void MotionController::update() {
    if (!_isMoving || !waypoints || waypointCount == 0) {
        driveTrain->stop();
        return;
    }

    if (currentWaypointIndex >= waypointCount) {
        stop();
        return;
    }

    // Check if waypoint reached
    if (isWaypointReached()) {
        currentWaypointIndex++;
        if (currentWaypointIndex >= waypointCount) {
            stop();
            return;
        }
    }

    // Calculate steering correction
    unsigned long now = millis();
    float dt = (now - pidLastTime) / 1000.0f;
    pidLastTime = now;

    Pose currentPose = poseEstimator->getPose();
    Waypoint target = waypoints[currentWaypointIndex];

    // Calculate angle to target
    float dx = target.x - currentPose.x;
    float dy = target.y - currentPose.y;
    float angleToTarget = atan2(dy, dx);

    // Calculate cross-track error
    float headingError = angleToTarget - currentPose.theta;

    // Normalize angle to [-PI, PI]
    while (headingError > M_PI) headingError -= 2.0f * M_PI;
    while (headingError < -M_PI) headingError += 2.0f * M_PI;

    // PID steering control
    float steeringCommand = calculatePidSteering(headingError, dt);

    // Move forward and steer
    driveTrain->setMotion(MaxLinearSpeedMmS * 0.5f, steeringCommand);
}

float MotionController::calculatePidSteering(float crossTrackError, float dt) {
    pidError = crossTrackError;
    pidIntegral += pidError * dt;
    pidIntegral = constrain(pidIntegral, -1.0f, 1.0f); // Anti-windup

    float derivative = 0;
    if (dt > 0) {
        derivative = (pidError - pidLastError) / dt;
    }
    pidLastError = pidError;

    float steering = (PidKp * pidError) + (PidKi * pidIntegral) + (PidKd * derivative);
    return constrain(steering, -MaxAngularSpeedRadS, MaxAngularSpeedRadS);
}

float MotionController::distanceToWaypoint() {
    if (currentWaypointIndex >= waypointCount) return 1e6f;

    Pose pose = poseEstimator->getPose();
    Waypoint wp = waypoints[currentWaypointIndex];

    float dx = wp.x - pose.x;
    float dy = wp.y - pose.y;
    return sqrt(dx * dx + dy * dy);
}

bool MotionController::isWaypointReached() {
    return distanceToWaypoint() <= waypoints[currentWaypointIndex].tolerance;
}

void MotionController::setWaypoints(Waypoint* wp, int count) {
    if (waypoints) delete[] waypoints;
    waypoints = new Waypoint[count];
    for (int i = 0; i < count; i++) {
        waypoints[i] = wp[i];
    }
    waypointCount = count;
    currentWaypointIndex = 0;
}

void MotionController::start() {
    _isMoving = true;
    pidLastTime = millis();
}

void MotionController::stop() {
    _isMoving = false;
    driveTrain->stop();
}

void MotionController::driveToWaypoint(float targetX, float targetY, float tolerance) {
    Waypoint wp = {targetX, targetY, tolerance};
    setWaypoints(&wp, 1);
    start();
}

void MotionController::reset() {
    stop();
    currentWaypointIndex = 0;
    pidError = 0;
    pidIntegral = 0;
    pidLastError = 0;
}
