#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <Arduino.h>
#include "Config.h"

class BluetoothManager {
private:
    bool isConnected = false;
    unsigned long lastPacketTime = 0;

public:
    BluetoothManager();
    ~BluetoothManager() = default;

    // Initialize BLE (Bluetooth Low Energy)
    bool begin();

    // Send debug packet over BLE
    void sendDebugPacket(const char* data);

    // Receive debug command
    void processIncomingCommand(const uint8_t* data, size_t length);

    // Check connection status
    bool isClientConnected() const { return isConnected; }

    // Stop BLE
    void stop();
};

#endif // BLUETOOTHMANAGER_H
