#include "BatteryMonitor.h"
#include <Arduino.h>

BatteryMonitor::BatteryMonitor() : lastVoltage(0), isLow(false), lastReadTime(0) {}

void BatteryMonitor::begin() {
    if (PinBatteryAdc > 0) {
        analogSetAttenuation(ADC_0db);
    }
    update();
}

void BatteryMonitor::update() {
    unsigned long now = millis();
    if (now - lastReadTime < 1000) return; // Update every 1 second

    lastReadTime = now;

    if (PinBatteryAdc <= 0) {
        lastVoltage = 8.0f; // Default safe voltage
        return;
    }

    // Read ADC and convert to voltage
    int adcValue = analogRead(PinBatteryAdc);
    lastVoltage = adcToVoltage(adcValue);

    // Check if low
    isLow = (lastVoltage < BatteryLowVoltage);

    if (isLow) {
        Serial.printf("[Battery] ⚠️  LOW BATTERY: %.2f V\n", lastVoltage);
    }
}

float BatteryMonitor::adcToVoltage(int adcReading) {
    // ESP32 ADC is 0-4095 for 0-3.3V
    // With divider ratio, actual battery voltage is higher
    float adcVoltage = (adcReading / 4095.0f) * 3.3f;
    float batteryVoltage = adcVoltage * BatteryDividerRatio;
    return batteryVoltage;
}

float BatteryMonitor::getPercentage() const {
    // Rough estimate for 4S LiPo (2.5V - 4.2V per cell = 10V - 16.8V)
    float minVoltage = 10.0f;
    float maxVoltage = 16.8f;

    if (lastVoltage <= minVoltage) return 0.0f;
    if (lastVoltage >= maxVoltage) return 100.0f;

    return ((lastVoltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0f;
}

void BatteryMonitor::printDiagnostics() {
    Serial.println("[Battery Diagnostics]");
    Serial.printf("  Voltage: %.2f V\n", lastVoltage);
    Serial.printf("  Percentage: %.1f%%\n", getPercentage());
    Serial.printf("  Status: %s\n", isLow ? "⚠️  LOW" : "✅ OK");
}
