# GiRobot Step 0: Foundation & Project Review

> **Phase 0**: Establish project structure, verify build configuration, understand module organization, and confirm compilation.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand the complete project structure and module dependencies
2. ✅ Verify PlatformIO build configuration for `esp32_real` and `esp32_sim` modes
3. ✅ Confirm firmware compiles without errors
4. ✅ Validate all header files and dependencies
5. ✅ Review Config.h constants (pins, physical parameters)
6. ✅ Confirm you can upload to ESP32 via PlatformIO
7. ✅ Verify serial monitor communication (115200 baud)
8. ✅ Document build workflow for future reference

---

## 🗂️ Project Structure Breakdown

### Root Directory

```
robot/
├── girobot.md                    ← Implementation roadmap (this project)
├── coding.md                     ← Original planning document
├── platformio.ini                ← Build & upload configuration
├── robot.code-workspace          ← VS Code workspace file
└── script.js                     ← Legacy script (check if needed)
```

### Configuration & Hardware Definition

```
include/
├── Config.h                      ← **KEY FILE**: All pin numbers, constants, firmware version
├── AutoIMU.h                     ← IMU auto-detection helper
├── BatteryMonitor.h              ← Battery voltage ADC reading
├── CommsManager.h                ← WiFi AP & WebSocket server
├── ConfigManager.h               ← Persistent parameter storage (NVS)
├── DCMotor.h                     ← DC motor control with encoder feedback
├── DriveTrain.h                  ← Differential drive kinematics
├── IMotor.h                      ← Abstract motor interface
├── MotionController.h            ← PID path following (waypoint navigation)
├── MotorTestSimple.h             ← Simple motor test utility
├── Navigation.h                  ← IMU sensor fusion (gyro, accel, mag)
├── PathPlanner.h                 ← Geometry & text rendering engine
├── PoseEstimator.h               ← Odometry + IMU fusion for position tracking
├── RealStepper.h                 ← Hardware stepper (for stepper motors)
├── SelfTest.h                    ← System health check
└── VirtualStepper.h              ← Software stepper simulation
```

### Implementation Files

```
src/
├── main.cpp                      ← System orchestration, setup(), loop(), task creation
├── AutoIMU.cpp                   ← IMU auto-detection (I2C address scanning)
├── BatteryMonitor.cpp            ← ADC voltage reading
├── CommsManager.cpp              ← WiFi AP startup, WebSocket event handlers
├── ConfigManager.cpp             ← NVS (Non-Volatile Storage) persistence
├── DCMotor.cpp                   ← Motor PWM, direction control, encoder ISR
├── DriveTrain.cpp                ← Convert linear/angular velocity to wheel speeds
├── MotionController.cpp          ← Waypoint following, cross-track error PID
├── MotorTestSimple.cpp           ← Motor speed/direction tests
├── Navigation.cpp                ← Mahony filter sensor fusion, calibration
├── PathPlanner.cpp               ← Circle/triangle/text path generation
├── PoseEstimator.cpp             ← Odometry + IMU fusion, ghost pose
├── RealStepper.cpp               ← Stepper motor control (AccelStepper wrapper)
├── SelfTest.cpp                  ← GPIO validation, encoder checks
└── VirtualStepper.cpp            ← Simulated motor (software-only)
```

### Web Interface (Frontend)

```
data/
├── index.html                    ← Main dashboard page (TO BE CREATED in Phase 8)
├── style.css                     ← Dashboard styling (TO BE CREATED in Phase 8)
└── script.js                     ← WebSocket client (TO BE CREATED in Phase 8)
```

### Build & Deployment

```
scripts/
└── upload_all.py                 ← Python helper for batch compilation and upload
```

---

## 🔧 Understanding Config.h

[include/Config.h](../include/Config.h) is the **master configuration file** containing all hardware definitions. Let's review the key sections:

### 1. **Firmware Identification**
```cpp
constexpr char FirmwareVersion[] = "0.1.1";
```
This version is printed at boot to verify which build is running.

### 2. **Physical Robot Parameters**
```cpp
constexpr float WheelDiameterMm = 90.0f;      // Drive wheel size
constexpr float WheelBaseMm = 83.0f;          // Distance between wheels (track width)
constexpr float PenOffsetMm = 130.0f;         // Distance from axle center to pen tip
constexpr int StepsPerRev = 1070;             // Motor encoder resolution
constexpr float MaxLinearSpeedMmS = 120.0f;   // Forward/backward speed limit
constexpr float MaxAngularSpeedRadS = 1.6f;   // Rotation speed limit
```

**Why this matters**:
- **WheelDiameterMm**: Used to convert motor steps → distance traveled
- **WheelBaseMm**: Used in differential drive kinematics (wheel speed ratio for turning)
- **PenOffsetMm**: Pen is 130mm ahead of axle; all paths must account for this offset
- **StepsPerRev**: Encoder resolution; used for odometry calculations
- **Speed limits**: Safety constraints for motion commands

### 3. **Left Motor Pins (DC motor mode)**
```cpp
constexpr int PinMotorLeftIn1 = 17;   // Direction control bit 1
constexpr int PinMotorLeftIn2 = 16;   // Direction control bit 2
constexpr int PinMotorLeftEn  = 4;    // PWM enable (speed control)
```

**Typical usage in DCMotor.cpp**:
- `digitalWrite(PinMotorLeftIn1, HIGH)` + `digitalWrite(PinMotorLeftIn2, LOW)` = forward
- `digitalWrite(PinMotorLeftIn1, LOW)` + `digitalWrite(PinMotorLeftIn2, HIGH)` = backward
- `analogWrite(PinMotorLeftEn, 200)` = 200/255 PWM speed

### 4. **Right Motor Pins**
```cpp
constexpr int PinMotorRightIn1 = 19;
constexpr int PinMotorRightIn2 = 18;
constexpr int PinMotorRightEn  = 23;
```

### 5. **Encoder Pins (Quadrature A/B signals)**
```cpp
// Left encoder (attached to left motor)
constexpr int PinEncoderLeftA = 32;
constexpr int PinEncoderLeftB = 33;

// Right encoder (attached to right motor)
constexpr int PinEncoderRightA = 27;
constexpr int PinEncoderRightB = 14;
```

**How encoders work**:
- Channel A: pulse train during motor rotation
- Channel B: phase-shifted pulse (90° ahead)
- Logic: read both channels to determine rotation direction and count steps
- Used by DCMotor.cpp to verify commanded steps match actual encoder ticks

### 6. **Battery Monitoring (ADC)**
```cpp
constexpr int PinBatteryAdc = 0;          // ADC input pin (CURRENTLY NOT WORKING)
constexpr float BatteryDividerRatio = 2.0f;  // Voltage divider ratio
constexpr float BatteryLowVoltage = 6.6f;    // Threshold for low battery warning
```

**Issue**: GPIO 0 is not reliably reading battery voltage. Phase 11 will investigate alternatives (GPIO36, GPIO39).

### 7. **I2C Bus (IMU Communication)**
```cpp
constexpr int PinI2cSda = 21;  // Serial data (IMU sensor)
constexpr int PinI2cScl = 22;  // Serial clock (IMU sensor)
constexpr int ImuAddress = 0;  // Auto-detect (0 = scan addresses)
```

**Auto-detection logic** (Navigation.cpp):
- Scans I2C bus for devices
- Detects MPU9250 at address 0x1E
- Detects BNO055 at address 0x6B
- Prints detected addresses to serial monitor

### 8. **WiFi Access Point**
```cpp
constexpr char WifiSsid[] = "RobotWifi";
constexpr char WifiPassword[] = "penbot123";
constexpr char WifiApIp[] = "192.168.4.1";
```

**How to connect**:
1. Find WiFi network "RobotWifi" on your PC/phone
2. Password: `penbot123`
3. Access web dashboard at `http://192.168.4.1`

### 9. **System Modes**
```cpp
enum class SystemRunMode {
    Sim,   // Virtual motors, no hardware (for testing on desktop)
    Real   // Physical motor drivers and hardware feedback
};
```

---

## 📦 Build Configuration: platformio.ini

[platformio.ini](../platformio.ini) defines two build environments:

### Environment: `esp32_real` (Hardware Mode)
- **Purpose**: Compile firmware for actual ESP32 with motors, encoders, IMU
- **Build Flags**: `-DMODE_REAL=1`
- **Libraries**: AccelStepper, MPU9250, motor drivers
- **Upload Port**: COM3 (your USB/serial connection)

### Environment: `esp32_sim` (Simulation Mode)
- **Purpose**: Compile firmware for desktop testing (no hardware drivers)
- **Build Flags**: `-DMODE_SIM=1`
- **Libraries**: No AccelStepper, no MPU9250 needed
- **Useful for**: Testing logic without physical hardware

---

## 🏗️ Module Dependency Graph

```
                    main.cpp
                       |
        ┌──────────────┼──────────────┐
        |              |              |
    DCMotor ←──── DriveTrain ──→ Navigation
        |              |              |
        |         PoseEstimator ──────┤
        |              |              |
        └──────────────┼──────────────┘
                       |
               MotionController
                       |
                  PathPlanner
                       |
                  CommsManager
                   (WebSocket)
```

**Reading this graph**:
- **DCMotor**: Bottom layer, controls hardware pins
- **Navigation**: Reads IMU sensors via I2C
- **DriveTrain**: Uses DCMotor to implement differential drive
- **PoseEstimator**: Fuses DriveTrain odometry + Navigation heading
- **MotionController**: Uses PoseEstimator for waypoint following
- **PathPlanner**: Generates waypoints for MotionController
- **CommsManager**: Streams telemetry from all modules over WebSocket

---

## ✅ Verification Checklist: Step 0

### Subtask 0.1: Understand Module Files

- [ ] Read [include/Config.h](../include/Config.h) to line 90 (all constants defined)
- [ ] Note down these key values from Config.h:
  - [ ] Wheel diameter: ____ mm
  - [ ] Wheel base: ____ mm
  - [ ] Pen offset: ____ mm
  - [ ] Steps per rev: ____ steps
  - [ ] Left motor enable pin: GPIO ____
  - [ ] Right motor enable pin: GPIO ____
  - [ ] Left encoder A: GPIO ____
  - [ ] Right encoder B: GPIO ____
  - [ ] I2C SDA: GPIO ____
  - [ ] I2C SCL: GPIO ____
  - [ ] Battery ADC: GPIO ____
  - [ ] WiFi SSID: ______________
  - [ ] WiFi IP: ______________

### Subtask 0.2: Review platformio.ini

- [ ] Open [platformio.ini](../platformio.ini)
- [ ] Verify `esp32_real` environment is configured
- [ ] Verify `esp32_sim` environment is configured
- [ ] Confirm serial monitor baud rate is 115200
- [ ] Check that build flags include `-DMODE_REAL=1` or `-DMODE_SIM=1`

### Subtask 0.3: Build Verification (Real Mode)

- [ ] Open terminal in VS Code
- [ ] Run command:
  ```bash
  platformio run -e esp32_real
  ```
  Expected: ✅ Build succeeds, no compilation errors
  
  If errors occur:
  - Check that all header files are in `include/`
  - Verify #include paths in .cpp files match
  - Run `platformio update` to refresh libraries

### Subtask 0.4: Upload to ESP32

- [ ] Connect ESP32 via USB (should appear as COM3 or similar)
- [ ] Run command:
  ```bash
  platformio run -e esp32_real --target upload --upload-port COM3
  ```
  (Replace COM3 with your port if different)
  
  Expected: ✅ Upload completes, no errors

### Subtask 0.5: Serial Monitor Connection

- [ ] Open serial monitor at 115200 baud:
  ```bash
  platformio device monitor -p COM3 -b 115200
  ```
  
  Expected output on boot:
  ```
  [I] Firmware Version: 0.1.1
  [I] System Mode: REAL
  [I] Initializing WiFi AP "RobotWifi"...
  [I] IP Address: 192.168.4.1
  [I] Scanning I2C bus for IMU...
  [I] Found IMU at address 0x1E (MPU9250) or 0x6B (BNO055)
  [I] Motor initialization...
  [I] System ready!
  ```

- [ ] If you see `[E]` error messages:
  - Sensor not found → Check I2C wiring (GPIO 21, 22)
  - Motor pin errors → Verify motor pins in Config.h
  - WiFi errors → Check SPIFFS/LittleFS filesystem

### Subtask 0.6: Understand main.cpp Flow

- [ ] Open [src/main.cpp](../src/main.cpp) and review:
  - [ ] `setup()` function: initialization sequence
  - [ ] Global instance declarations (config, battery, nav, drive, pose, motion, comms)
  - [ ] Real vs Sim mode selection (#if defined(MODE_REAL))
  - [ ] Control task creation (`xTaskCreatePinnedToCore`)
  - [ ] `loop()` function: telemetry updates, cleanup timers

- [ ] Understand these key instances:
  - `ConfigManager config` → Loads all pin/parameter constants
  - `DriveTrain drive` → Controls both motors
  - `Navigation nav` → Reads IMU
  - `PoseEstimator pose` → Fuses odometry + IMU
  - `CommsManager comms` → WebSocket server

### Subtask 0.7: Quick Sanity Check

Run the SelfTest to validate pin configuration:

- [ ] Assuming you have serial monitor open, look for SelfTest output
- [ ] Check these items pass:
  - [ ] Pin range validation (all GPIO pins within valid range)
  - [ ] Encoder pins are different from motor pins
  - [ ] Battery ADC pin is valid or disabled (0)
  - [ ] WiFi config is valid

If SelfTest fails, check Config.h pin assignments.

---

## 🐛 Common Issues & Troubleshooting

### Issue: Build fails with "undefined reference to `DCMotor::DCMotor(...)`"

**Cause**: Linker can't find object files from src/

**Solution**:
```bash
platformio clean
platformio run -e esp32_real
```

### Issue: Serial monitor shows garbage characters

**Cause**: Baud rate mismatch

**Solution**:
- Verify serial monitor baud is 115200
- Check Config.h doesn't override this
- Re-upload firmware

### Issue: No serial output after upload

**Cause**: Monitor not reconnected after upload

**Solution**:
```bash
# First upload
platformio run -e esp32_real --target upload --upload-port COM3

# Then open monitor
platformio device monitor -p COM3 -b 115200
```

### Issue: I2C device not found

**Cause**: I2C pins not connected or pulled low

**Solution**:
- Verify GPIO 21 (SDA) connected to IMU SDA pin
- Verify GPIO 22 (SCL) connected to IMU SCL pin
- Check for pull-up resistors (typically 4.7k on I2C bus)

### Issue: ESP32 won't appear in COM ports

**Cause**: USB driver not installed

**Solution**:
- Install CH340 or CP2102 driver (depends on your USB chip)
- Restart VS Code and PlatformIO
- Try different USB cable

---

## 📚 Key Files Summary

| File | Purpose | Status |
|------|---------|--------|
| [include/Config.h](../include/Config.h) | Hardware pin definitions & constants | ✅ Reference only |
| [src/main.cpp](../src/main.cpp) | System orchestration | ✅ Review to understand flow |
| [platformio.ini](../platformio.ini) | Build configuration | ✅ Verify environments |
| [src/SelfTest.cpp](../src/SelfTest.cpp) | Hardware validation | ✅ Optional runtime check |

---

## 📝 Next Steps

Once Phase 0 is verified:

✅ **Phase 0 Complete**
→ Move to [girobot_step1.md](girobot_step1.md): **Motor & Encoder Control**

In Phase 1, you will:
1. Test motor speed ramps (acceleration/deceleration)
2. Verify encoder feedback matches commanded steps
3. Check for motor faults (speed mismatch > threshold)
4. Validate straight-line driving without encoder slippage

---

## 🎯 Summary

**Phase 0 accomplishes:**
- ✅ Documented project structure and module interdependencies
- ✅ Explained Config.h constants and their purpose
- ✅ Verified PlatformIO build configuration
- ✅ Confirmed firmware compiles and uploads successfully
- ✅ Tested serial communication and boot output
- ✅ Validated SelfTest diagnostics

**You are now ready for Phase 1: Motor & Encoder Control.**

---

**Estimated Time**: 1 hour  
**Difficulty**: ⭐ Beginner (review & verification only)  
**Hardware Required**: ESP32, USB cable, serial monitor  

Last Updated: May 11, 2026
