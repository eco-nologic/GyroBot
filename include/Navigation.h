#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <Arduino.h>
#include "Config.h"

struct ImuData {
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    float magX, magY, magZ;
    float temperature;
};

struct OrientationData {
    float roll;    // Rotation around X axis
    float pitch;   // Rotation around Y axis
    float heading; // Rotation around Z axis (yaw) - this is what we use most
};

class Navigation {
private:
    ImuData rawData;
    OrientationData orientation;
    float headingIntegral = 0.0f;
    unsigned long lastUpdate = 0;

    // Mahony filter state
    float fusionAlpha = 0.98f; // Blend factor (more gyro, less magnetometer)
    float gyroBiasx = 0, gyroBiasy = 0, gyroBiasz = 0;

    void updateMahonyFilter(float dt);
    void calibrateOffsets();
    bool detectImu();

public:
    Navigation();
    ~Navigation() = default;

    bool begin();
    void update();
    void calibrate();

    // Get current orientation
    OrientationData getOrientation() const { return orientation; }
    float getHeading() const { return orientation.heading; } // Radians

    // Get raw IMU data
    ImuData getRawData() const { return rawData; }

    // Health check
    bool isCalibrated() const;
};

#endif // NAVIGATION_H
