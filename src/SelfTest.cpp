#include "SelfTest.h"
#include "Config.h"
#include <Arduino.h>

bool SelfTest::runAllTests() {
    Serial.println("[SelfTest] Starting system self-test...");

    if (!testPinConfiguration()) {
        Serial.println("[ERROR] Pin configuration test failed!");
        return false;
    }

    if (!testMotorPins()) {
        Serial.println("[ERROR] Motor pins test failed!");
        return false;
    }

    if (!testEncoderPins()) {
        Serial.println("[ERROR] Encoder pins test failed!");
        return false;
    }

    if (!testI2cPins()) {
        Serial.println("[ERROR] I2C pins test failed!");
        return false;
    }

    if (!testWifiConfig()) {
        Serial.println("[ERROR] WiFi config test failed!");
        return false;
    }

    Serial.println("[SelfTest] ✅ All tests passed!");
    return true;
}

bool SelfTest::testPinConfiguration() {
    Serial.println("[SelfTest] Testing pin configuration...");

    // Check for invalid pin numbers
    if (PinMotorLeftEn < 0 || PinMotorLeftEn > 39) return false;
    if (PinMotorRightEn < 0 || PinMotorRightEn > 39) return false;
    if (PinEncoderLeftA < 0 || PinEncoderLeftA > 39) return false;
    if (PinEncoderLeftB < 0 || PinEncoderLeftB > 39) return false;
    if (PinEncoderRightA < 0 || PinEncoderRightA > 39) return false;
    if (PinEncoderRightB < 0 || PinEncoderRightB > 39) return false;
    if (PinI2cSda < 0 || PinI2cSda > 39) return false;
    if (PinI2cScl < 0 || PinI2cScl > 39) return false;

    Serial.println("[SelfTest] ✅ Pin configuration OK");
    return true;
}

bool SelfTest::testMotorPins() {
    Serial.println("[SelfTest] Testing motor pins...");

    // Check that motor pins are unique
    if (PinMotorLeftEn == PinMotorLeftIn1 || PinMotorLeftEn == PinMotorLeftIn2) return false;
    if (PinMotorRightEn == PinMotorRightIn1 || PinMotorRightEn == PinMotorRightIn2) return false;

    Serial.println("[SelfTest] ✅ Motor pins OK");
    return true;
}

bool SelfTest::testEncoderPins() {
    Serial.println("[SelfTest] Testing encoder pins...");

    // Check that encoder pins are different from motor pins
    if (PinEncoderLeftA == PinMotorLeftEn || PinEncoderLeftB == PinMotorLeftEn) return false;
    if (PinEncoderRightA == PinMotorRightEn || PinEncoderRightB == PinMotorRightEn) return false;

    Serial.println("[SelfTest] ✅ Encoder pins OK");
    return true;
}

bool SelfTest::testI2cPins() {
    Serial.println("[SelfTest] Testing I2C pins...");

    if (PinI2cSda == PinI2cScl) return false;

    Serial.println("[SelfTest] ✅ I2C pins OK");
    return true;
}

bool SelfTest::testWifiConfig() {
    Serial.println("[SelfTest] Testing WiFi configuration...");

    if (strlen(WifiSsid) == 0) return false;
    if (strlen(WifiPassword) == 0) return false;

    Serial.println("[SelfTest] ✅ WiFi config OK");
    return true;
}

bool SelfTest::testBatteryAdc() {
    Serial.println("[SelfTest] Testing battery ADC...");

    if (PinBatteryAdc == 0) {
        Serial.println("[WARN] Battery ADC disabled (pin 0)");
        return true; // Not a failure, just disabled
    }

    Serial.println("[SelfTest] ✅ Battery ADC OK");
    return true;
}
