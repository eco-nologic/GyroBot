#include "PathPlanner.h"
#include <math.h>

PathPlanner::PathPlanner(MotionController* mc) : motionController(mc) {}

void PathPlanner::begin() {
    clearPath();
}

void PathPlanner::drawCircle(float centerX, float centerY, float radiusMm, int segments) {
    generatedWaypoints.clear();

    for (int i = 0; i < segments; i++) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = centerX + radiusMm * cos(angle);
        float y = centerY + radiusMm * sin(angle);
        generatedWaypoints.push_back({x, y, 20.0f}); // 20mm tolerance
    }

    // Return to start
    generatedWaypoints.push_back({centerX + radiusMm, centerY, 20.0f});
}

void PathPlanner::drawTriangle(float centerX, float centerY, float sideLength) {
    generatedWaypoints.clear();

    float radius = sideLength / sqrt(3.0f);

    for (int i = 0; i < 3; i++) {
        float angle = (2.0f * M_PI * i) / 3.0f - M_PI / 2.0f;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        generatedWaypoints.push_back({x, y, 20.0f});
    }

    // Return to start
    float angle = -M_PI / 2.0f;
    float x = centerX + radius * cos(angle);
    float y = centerY + radius * sin(angle);
    generatedWaypoints.push_back({x, y, 20.0f});
}

void PathPlanner::drawRectangle(float centerX, float centerY, float width, float height) {
    generatedWaypoints.clear();

    float halfW = width / 2.0f;
    float halfH = height / 2.0f;

    generatedWaypoints.push_back({centerX - halfW, centerY - halfH, 20.0f});
    generatedWaypoints.push_back({centerX + halfW, centerY - halfH, 20.0f});
    generatedWaypoints.push_back({centerX + halfW, centerY + halfH, 20.0f});
    generatedWaypoints.push_back({centerX - halfW, centerY + halfH, 20.0f});
    generatedWaypoints.push_back({centerX - halfW, centerY - halfH, 20.0f});
}

void PathPlanner::drawText(const char* text, float startX, float startY, float scale) {
    // Simplified text rendering - would need actual font data
    generatedWaypoints.clear();

    // For now, just place a marker at text position
    generatedWaypoints.push_back({startX, startY, 50.0f});
}

void PathPlanner::generateBezierCurve(float p0x, float p0y, float p1x, float p1y,
                                       float p2x, float p2y, float p3x, float p3y,
                                       int steps) {
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1.0f - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;

        float x = mt3 * p0x + 3.0f * mt2 * t * p1x + 3.0f * mt * t2 * p2x + t3 * p3x;
        float y = mt3 * p0y + 3.0f * mt2 * t * p1y + 3.0f * mt * t2 * p2y + t3 * p3y;

        generatedWaypoints.push_back({x, y, 10.0f});
    }
}

void PathPlanner::executePath() {
    if (generatedWaypoints.empty()) return;

    // Convert vector to array and pass to motion controller
    Waypoint* wpArray = new Waypoint[generatedWaypoints.size()];
    for (size_t i = 0; i < generatedWaypoints.size(); i++) {
        wpArray[i] = generatedWaypoints[i];
    }

    motionController->setWaypoints(wpArray, generatedWaypoints.size());
    motionController->start();

    delete[] wpArray;
}

void PathPlanner::clearPath() {
    generatedWaypoints.clear();
}
