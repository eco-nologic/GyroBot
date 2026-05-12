#ifndef SELFTEST_H
#define SELFTEST_H

#include <Arduino.h>

class SelfTest {
public:
    // Run complete system self-test
    static bool runAllTests();

    // Individual tests
    static bool testPinConfiguration();
    static bool testMotorPins();
    static bool testEncoderPins();
    static bool testI2cPins();
    static bool testWifiConfig();
    static bool testBatteryAdc();
};

#endif // SELFTEST_H
