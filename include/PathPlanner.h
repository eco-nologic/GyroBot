#ifndef PATHPLANNER_H
#define PATHPLANNER_H

#include <Arduino.h>
#include "Config.h"
#include "MotionController.h"
#include <vector>

class PathPlanner {
private:
    MotionController* motionController;
    std::vector<Waypoint> generatedWaypoints;

    // Bezier curve approximation for smooth text
    void generateBezierCurve(float p0x, float p0y, float p1x, float p1y,
                             float p2x, float p2y, float p3x, float p3y,
                             int steps);

public:
    PathPlanner(MotionController* mc);
    ~PathPlanner() = default;

    void begin();

    // Draw a circle
    void drawCircle(float centerX, float centerY, float radiusMm, int segments = 32);

    // Draw a triangle
    void drawTriangle(float centerX, float centerY, float sideLength);

    // Draw a rectangle
    void drawRectangle(float centerX, float centerY, float width, float height);

    // Draw text (simplified - Bezier curves)
    void drawText(const char* text, float startX, float startY, float scale = 1.0f);

    // Execute path (send waypoints to motion controller)
    void executePath();

    // Clear generated path
    void clearPath();

    // Get waypoint count
    int getWaypointCount() const { return generatedWaypoints.size(); }
};

#endif // PATHPLANNER_H
