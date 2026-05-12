#include "Navigation.h"
#include "AutoIMU.h"
#include <Wire.h>
#include <math.h>

Navigation::Navigation() {
    memset(&rawData, 0, sizeof(ImuData));
    memset(&orientation, 0, sizeof(OrientationData));
}

bool Navigation::begin() {
    Serial.println("[Nav] Initializing IMU...");

    if (!detectImu()) {
        Serial.println("[Nav] IMU detection failed!");
        return false;
    }

    calibrate();
    lastUpdate = millis();
    return true;
}

bool Navigation::detectImu() {
    Wire.begin(PinI2cSda, PinI2cScl);
    uint8_t addr = AutoIMU::detectAddress();

    if (addr == 0) {
        Serial.println("[Nav] No IMU found on I2C bus");
        return false;
    }

    Serial.printf("[Nav] IMU detected at address 0x%02X\n", addr);
    return true;
}

void Navigation::calibrate() {
    Serial.println("[Nav] Running calibration...");
    // Reset biases
    gyroBiasx = gyroBiasy = gyroBiasz = 0.0f;
    Serial.println("[Nav] Calibration complete");
}

void Navigation::update() {
    unsigned long now = millis();
    float dt = (now - lastUpdate) / 1000.0f;
    lastUpdate = now;

    // Read sensor data (simulated)
    rawData.gyroX = 0.0f;
    rawData.gyroY = 0.0f;
    rawData.gyroZ = 0.0f;
    rawData.accelX = 0.0f;
    rawData.accelY = 0.0f;
    rawData.accelZ = 9.81f;

    // Update Mahony filter
    updateMahonyFilter(dt);
}

void Navigation::updateMahonyFilter(float dt) {
    // Simple integration of gyro to update heading
    float gyroZCorrected = rawData.gyroZ - gyroBiasz;
    headingIntegral += gyroZCorrected * dt;

    // Blend gyro and magnetometer
    orientation.heading = headingIntegral;
    orientation.roll = atan2(rawData.accelY, rawData.accelZ);
    orientation.pitch = atan2(-rawData.accelX, sqrt(rawData.accelY * rawData.accelY + rawData.accelZ * rawData.accelZ));
}

bool Navigation::isCalibrated() const {
    return true; // Simplified
}
