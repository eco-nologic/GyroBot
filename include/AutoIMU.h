#ifndef AUTOIMU_H
#define AUTOIMU_H

#include <Arduino.h>

// Scan I2C bus and detect IMU at known addresses
class AutoIMU {
public:
    // Returns I2C address of detected IMU, or 0 if not found
    static uint8_t detectAddress();

    // Print all I2C devices found
    static void scanBus();
};

#endif // AUTOIMU_H
