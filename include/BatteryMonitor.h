#ifndef BATTERYMONITOR_H
#define BATTERYMONITOR_H

#include <Arduino.h>
#include "Config.h"

class BatteryMonitor {
private:
    float lastVoltage = 0.0f;
    bool isLow = false;
    unsigned long lastReadTime = 0;

    // Convert ADC reading to voltage
    float adcToVoltage(int adcReading);

public:
    BatteryMonitor();
    ~BatteryMonitor() = default;

    void begin();
    void update();

    // Get current battery voltage (volts)
    float getVoltage() const { return lastVoltage; }

    // Check if battery is low
    bool isLowBattery() const { return isLow; }

    // Get battery percentage (rough estimate)
    float getPercentage() const;

    // Debug: print battery info
    void printDiagnostics();
};

#endif // BATTERYMONITOR_H
