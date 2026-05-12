#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include <Arduino.h>
#include "Config.h"
#include "DriveTrain.h"
#include "PoseEstimator.h"

struct Waypoint {
    float x;
    float y;
    float tolerance; // Radius of acceptance (mm)
};

class MotionController {
private:
    DriveTrain* driveTrain;
    PoseEstimator* poseEstimator;

    Waypoint* waypoints = nullptr;
    int waypointCount = 0;
    int currentWaypointIndex = 0;

    bool _isMoving = false;

    // PID controller state
    float pidError = 0;
    float pidIntegral = 0;
    float pidLastError = 0;
    unsigned long pidLastTime = 0;

    // Calculate steering correction from cross-track error
    float calculatePidSteering(float crossTrackError, float dt);

    // Calculate distance to current waypoint
    float distanceToWaypoint();

    // Check if waypoint is reached
    bool isWaypointReached();

public:
    MotionController(DriveTrain* drive, PoseEstimator* pose);
    ~MotionController() { if (waypoints) delete[] waypoints; }

    void begin();
    void update();

    // Set waypoint list for path following
    void setWaypoints(Waypoint* wp, int count);

    // Start/stop motion
    void start();
    void stop();

    // Commanding direct motion
    void driveToWaypoint(float targetX, float targetY, float tolerance = 50.0f);

    // Status
    bool isMoving() const { return _isMoving; }
    int getCurrentWaypoint() const { return currentWaypointIndex; }
    int getTotalWaypoints() const { return waypointCount; }
    Waypoint getCurrentTargetWaypoint() const {
        if (_isMoving && waypoints && currentWaypointIndex < waypointCount) {
            return waypoints[currentWaypointIndex];
        }
        // Return a default/invalid waypoint if not moving or no target
        return {0.0f, 0.0f, 0.0f}; 
    }

    // Stop and reset
    void reset();
};

#endif // MOTIONCONTROLLER_H
