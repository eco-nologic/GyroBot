#include "AutoIMU.h"
#include <Wire.h>
#include "Config.h"

uint8_t AutoIMU::detectAddress() {
    Serial.println("[AutoIMU] Scanning I2C bus...");

    Wire.begin(PinI2cSda, PinI2cScl);

    // Known IMU addresses
    const uint8_t addresses[] = {0x1E, 0x6B}; // MPU9250, BNO055
    const char* names[] = {"MPU9250", "BNO055"};

    for (int i = 0; i < 2; i++) {
        Wire.beginTransmission(addresses[i]);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[AutoIMU] Found %s at 0x%02X\n", names[i], addresses[i]);
            return addresses[i];
        }
    }

    Serial.println("[AutoIMU] No IMU found!");
    return 0;
}

void AutoIMU::scanBus() {
    Serial.println("[AutoIMU] Full I2C bus scan:");

    Wire.begin(PinI2cSda, PinI2cScl);

    int foundCount = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] Device found at 0x%02X\n", addr);
            foundCount++;
        }
    }

    if (foundCount == 0) {
        Serial.println("[I2C] No devices found!");
    }
}
