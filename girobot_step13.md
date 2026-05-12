# GiRobot Step 13: Bluetooth AI Debug Bridge

> **Phase 13**: Implement a high-performance Bluetooth Low Energy (BLE) interface using NimBLE to provide a dedicated data link for AI-assisted diagnostics and out-of-band control.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Integrate the `NimBLE-Arduino` library for memory-efficient BLE operations.
2. ✅ Implement `BluetoothManager` to manage advertising and direct AI connections.
3. ✅ Define a **Debug Service** with characteristics for commands and telemetry.
4. ✅ Bridge BLE incoming data to the existing `MotionController` logic.
5. ✅ Stream state at 10Hz using **Short-Key JSON** to minimize token cost and latency.
6. ✅ Verify communication using an AI agent or a BLE terminal.

---

## 📉 AI-Optimized Protocol (Short-Key JSON)

To maximize the efficiency of an external AI debugger, we use compressed JSON keys over the BLE link:

| Key | Name | Description |
| :--- | :--- | :--- |
| **c** | cmd | Command name (e.g., "D" for Drive, "S" for Stop) |
| **v** | linear | Linear velocity in mm/s |
| **w** | angular | Angular velocity in rad/s |
| **x** | pen_x | Current X position in mm |
| **y** | pen_y | Current Y position in mm |
| **h** | heading | Current heading in radians |
| **b** | battery | Battery voltage (V) |

---

## 🔧 Implementation Sketch: BluetoothManager

### 1. Service and UUID Configuration

**Service UUID**: `abcd0001-1234-4321-abcd-0123456789ab`  
**Command Char (Write)**: `abcd0002-...`  
**Telemetry Char (Notify)**: `abcd0003-...`

### 2. Header Skeleton (`BluetoothManager.h`)

```cpp
#include <NimBLEDevice.h>

class BluetoothManager {
public:
    void begin();
    void sendState(float x, float y, float h, float b);
    bool hasClient() { return connectedCount > 0; }

private:
    NimBLECharacteristic* pCmdChar;
    NimBLECharacteristic* pTelemChar;
    int connectedCount = 0;

    class ServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };

    class CommandCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    };
};
```

### 3. Dispatch Logic
The `onWrite` callback parses the short-key JSON and interacts with the `MotionController`:

```cpp
void BluetoothManager::CommandCallbacks::onWrite(NimBLECharacteristic* pChar) {
    std::string val = pChar->getValue();
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, val) == DeserializationError::Ok) {
        const char* cmd = doc["c"];
        if (strcmp(cmd, "D") == 0) {
            motion.drive(doc["v"], doc["w"]);
        } else if (strcmp(cmd, "S") == 0) {
            motion.stop();
        }
    }
}
```

---

## 🧪 Testing Procedure

### Test 1: Discovery & MTU
**Expected**: Device appears as "GiRobot_Debug" and supports MTU negotiation for large JSON packets.
1. Scan for devices using **nRF Connect**.
2. Connect and verify Service `abcd0001` is visible.

### Test 2: AI Command Injection
**Expected**: Robot moves forward when receiving a compressed command.
1. Connect via BLE Terminal.
2. Send: `{"c":"D","v":60,"w":0}`.
3. **Verification**: Observe robot moving forward immediately.

### Test 3: Telemetry Notifications
**Expected**: Continuous stream of state data.
1. Enable "Notifications" on characteristic `abcd0003`.
2. **Verification**: Verify receiving `{"x":10.2,"y":5.1,"h":0.78,"b":7.4}` at 10Hz.

---

## 🤖 AI Debugging Workflow

If the robot behaves erratically, the AI debugger can:
1. **Connect**: Establish a stable link even if WiFi is saturated.
2. **Monitor**: Check the `h` (heading) value to see if the IMU is drifting beyond thresholds.
3. **Intervene**: Send `{"c":"S"}` to stop the robot before it hits an obstacle.
4. **Tune**: Send PID adjustment commands (e.g., `{"c":"PID","kp":0.4}`) to optimize tracking in real-time.

---

## ✅ Verification Checklist: Step 13

- [ ] **NimBLE Integration**: Compiles and runs on ESP32 Core 0 without blocking.
- [ ] **Advertising**: Correctly identifies as "GiRobot_Debug".
- [ ] **Short-Key Parsing**: Successfully parses and executes commands using `c`, `v`, and `w`.
- [ ] **Notification Flow**: Telemetry is pushed via notifications only when a client is connected.
- [ ] **Memory Footprint**: BLE stack doesn't cause heap exhaustion during WiFi/WebSocket usage.

---

**Estimated Time**: 2 hours
**Difficulty**: ⭐⭐⭐ Advanced (BLE stack and thread safety)
**Hardware Required**: ESP32, BLE-capable smartphone or PC

Last Updated: May 11, 2026