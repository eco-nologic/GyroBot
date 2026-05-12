#include "ConfigManager.h"

ConfigManager::ConfigManager() : nvsHandle(0) {}

bool ConfigManager::begin() {
    esp_err_t err = nvs_open("robot", NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        Serial.printf("[Config] NVS init failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("[Config] NVS initialized");
    return true;
}

void ConfigManager::setFloat(const char* key, float value) {
    if (nvsHandle == 0) return;
    nvs_set_blob(nvsHandle, key, &value, sizeof(float));
    nvs_commit(nvsHandle);
}

void ConfigManager::setInt(const char* key, int32_t value) {
    if (nvsHandle == 0) return;
    nvs_set_i32(nvsHandle, key, value);
    nvs_commit(nvsHandle);
}

void ConfigManager::setString(const char* key, const char* value) {
    if (nvsHandle == 0) return;
    nvs_set_str(nvsHandle, key, value);
    nvs_commit(nvsHandle);
}

float ConfigManager::getFloat(const char* key, float defaultValue) {
    if (nvsHandle == 0) return defaultValue;

    float value = defaultValue;
    size_t size = sizeof(float);
    nvs_get_blob(nvsHandle, key, &value, &size);
    return value;
}

int32_t ConfigManager::getInt(const char* key, int32_t defaultValue) {
    if (nvsHandle == 0) return defaultValue;

    int32_t value;
    esp_err_t err = nvs_get_i32(nvsHandle, key, &value);
    if (err == ESP_OK) return value;
    return defaultValue;
}

String ConfigManager::getString(const char* key, const char* defaultValue) {
    if (nvsHandle == 0) return String(defaultValue);

    static char buffer[256];
    size_t size = sizeof(buffer);
    esp_err_t err = nvs_get_str(nvsHandle, key, buffer, &size);
    if (err == ESP_OK) return String(buffer);
    return String(defaultValue);
}

void ConfigManager::resetToDefaults() {
    if (nvsHandle == 0) return;
    nvs_erase_all(nvsHandle);
    nvs_commit(nvsHandle);
    Serial.println("[Config] Reset to defaults");
}

void ConfigManager::printAll() {
    Serial.println("[Config] Stored values:");
    // NVS iterator not easily available without more complex code
    Serial.println("  (Use NVS explorer for full list)");
}
