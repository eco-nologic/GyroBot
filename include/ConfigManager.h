#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>
#include "Config.h"
#include <nvs_flash.h>
#include <nvs.h>

class ConfigManager {
private:
    nvs_handle_t nvsHandle;

public:
    ConfigManager();
    ~ConfigManager() = default;

    // Initialize NVS (Non-Volatile Storage)
    bool begin();

    // Save parameter
    void setFloat(const char* key, float value);
    void setInt(const char* key, int32_t value);
    void setString(const char* key, const char* value);

    // Load parameter
    float getFloat(const char* key, float defaultValue = 0.0f);
    int32_t getInt(const char* key, int32_t defaultValue = 0);
    String getString(const char* key, const char* defaultValue = "");

    // Reset to defaults
    void resetToDefaults();

    // Print all stored values
    void printAll();
};

#endif // CONFIGMANAGER_H
