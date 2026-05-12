# GiRobot: Complete Step-by-Step Implementation Plan

> **A precision pen-drawing robot with differential drive, IMU navigation, and web-based control**

---

## 🎯 Project Goals

Build a fully autonomous differential-drive robot that:
1. ✅ Controls two rear motors with encoder feedback
2. ✅ Integrates 9-axis IMU (gyro, accel, magnetometer) for accurate heading
3. ✅ Fuses wheel odometry + IMU data for robust position tracking
4. ✅ Executes precise motion commands: go straight, turn, follow paths
5. ✅ Draws geometric shapes (circles, triangles, rectangles)
6. ✅ Writes text with font rendering and Bezier smoothing
7. ✅ Streams telemetry over WebSocket to web dashboard
8. ✅ Displays real-time robot position, path history, and sensor data
9. ✅ Provides web-based control interface (joystick, geometry buttons, text input)
10. ✅ Tracks both actual path and ghost path (pure odometry) for visualization
11. ✅ Supports pen features (contact detection, lift logic)
12. ✅ Fixes battery voltage monitoring
13. ✅ Implements Bluetooth AI Debug Bridge for out-of-band diagnostics

---

## 📋 Implementation Phases (13 steps)

Each phase has:
- **Detailed step file** with code snippets, pin definitions, and algorithms
- **Verification checklist** to confirm successful completion
- **Hardware reference** from Config.h constants
- **Integration points** with other modules

### Phase Overview

| Phase | File | Focus | Dependencies | Est. Time |
|-------|------|-------|--------------|-----------|
| 0 | [girobot_step0.md](girobot_step0.md) | Foundation & Project Structure | None | 1 hr |
| 1 | [girobot_step1.md](girobot_step1.md) | Motor & Encoder Control | Phase 0 | 2 hrs |
| 2 | [girobot_step2.md](girobot_step2.md) | IMU Sensor Integration | Phase 0 | 2 hrs |
| 3 | [girobot_step3.md](girobot_step3.md) | Odometry & Position Tracking | Phase 1, 2 | 2 hrs |
| 4 | [girobot_step4.md](girobot_step4.md) | Motion Commands & PID Enhancement | Phase 3 | 3 hrs |
| 5 | [girobot_step5.md](girobot_step5.md) | Pen Features & Drawing Logic | Phase 4 | 1.5 hrs |
| 6 | [girobot_step6.md](girobot_step6.md) | Geometry & Text Rendering | Phase 4 | 1 hr |
| 7 | [girobot_step7.md](girobot_step7.md) | WebSocket Server Backend | Phase 0 | 2 hrs |
| 8 | [girobot_step8.md](girobot_step8.md) | Web Frontend (HTML/CSS/JS) | Phase 7 | 3 hrs |
| 9 | [girobot_step9.md](girobot_step9.md) | Web Controls & Extended Commands | Phase 8 | 2 hrs |
| 10 | [girobot_step10.md](girobot_step10.md) | Map & Ghost Map Display | Phase 8, 3 | 2 hrs |
| 11 | [girobot_step11.md](girobot_step11.md) | Battery Diagnostics & Fix | Phase 0 | 1.5 hrs |
| 12 | [girobot_step12.md](girobot_step12.md) | Integration Testing & Tuning | All phases | 3 hrs |
| 13 | [girobot_step13.md](girobot_step13.md) | Bluetooth AI Debug Bridge | Phase 7 | 2 hrs |

Notice than step13 could be used for DEBUG, so maybe could be done earlier
---

## 🔧 Quick Hardware Reference

**From [include/Config.h](include/Config.h):**

### Physical Constants
```
Wheel Diameter:        90.0 mm
Wheel Base:            83.0 mm
Pen Offset:            130.0 mm (distance from axle to pen tip)
Steps per Revolution:  1070 steps
Max Linear Speed:      120.0 mm/s
Max Angular Speed:     1.6 rad/s
Firmware Version:      0.1.1
```

### Motor Pins (DC motors)
```
Left Motor:
  - Enable (PWM):      GPIO 4
  - Direction IN1:     GPIO 17
  - Direction IN2:     GPIO 16

Right Motor:
  - Enable (PWM):      GPIO 23
  - Direction IN1:     GPIO 19
  - Direction IN2:     GPIO 18
```

### Encoder Pins (with A/B channels)
```
Left Encoder:
  - Channel A:         GPIO 32
  - Channel B:         GPIO 33

Right Encoder:
  - Channel A:         GPIO 27
  - Channel B:         GPIO 14
```

### I2C (IMU: MPU9250 or BNO055)
```
SDA:                   GPIO 21
SCL:                   GPIO 22
IMU Address:           0x1E (MPU9250) or 0x6B (BNO055), auto-detect
```

### Battery Monitoring
```
ADC Pin:               GPIO 0 (currently disabled/not working)
Divider Ratio:         2.0x (battery voltage / ADC input)
Low Voltage Threshold: 6.6V
```

### WiFi Access Point
```
SSID:                  "RobotWifi"
Password:              "penbot123"
IP Address:            192.168.4.1
```

---

## 📊 Current Implementation Status

| Module | Status | Notes |
|--------|--------|-------|
| **DCMotor.cpp** | ✅ FULL | Encoder fault detection, PWM deadband (151-255) |
| **VirtualStepper.cpp** | ✅ FULL | Simulation support, no hardware pins |
| **DriveTrain.cpp** | ✅ FULL | Differential drive kinematics |
| **Navigation.cpp** | ✅ FULL | IMU sensor fusion (MPU9250, BNO055), auto-detect |
| **PoseEstimator.cpp** | ✅ FULL | Odometry + IMU fusion, ghost pose tracking |
| **MotionController.cpp** | ⚠️ PARTIAL | Waypoint following basic; **needs full PID enhancement (Ki, Kd terms)** |
| **PathPlanner.cpp** | ✅ FULL | Shapes (circle, triangle, rect) + text with Bezier |
| **CommsManager.cpp** | ✅ FULL | WiFi AP, WebSocket (Port 80), JSON telemetry |
| **ConfigManager.cpp** | ✅ FULL | NVS persistent storage, 20+ params |
| **BatteryMonitor.cpp** | ⚠️ PARTIAL | ADC reading structure exists; **ADC pin 0 not working** |

**Summary**: 9 out of 10 modules are production-ready. WebSocket and Web Server now share Port 80 for simplicity and to avoid browser cross-origin issues.

---

## 🚀 Quick Start

1. **Phase 0**: Review project structure, verify build
2. **Phase 1-2**: Confirm motors and IMU work (hardware tests)
3. **Phase 3-4**: Verify odometry and motion accuracy
4. **Phase 5-6**: Validate drawing and text features
5. **Phase 7-10**: Build and test web interface
6. **Phase 11**: Fix battery ADC
7. **Phase 12**: Full integration test
8. **Phase 13**: Setup BLE Debug Bridge

---

## 📐 Physical Geometry (Pen-Centric Model)

```
                 Pen (0, 0)           ← Front / Origin / Triangle Summit
                   /    \
                  /      \
                 /        \
           Left Wheel --- Right Wheel   ← Rear axle center
         (-D, -L/2)      (-D, +L/2)

Key:
- D = 130mm (pen offset from axle center)
- L = 83mm (wheel base / track width)
```

**Critical Physics**: The pen is 130mm ahead of the drive axle. When the robot turns, the pen traces a larger arc than the wheels. All path planning must account for this offset.

---

## 🔗 File Structure

```
robot/
├── girobot.md                    ← This file (overview)
├── girobot_step0.md              ← Foundation & setup
├── girobot_step1.md              ← Motors & encoders
├── girobot_step2.md              ← IMU integration
├── girobot_step3.md              ← Odometry & position
├── girobot_step4.md              ← Motion & PID
├── girobot_step5.md              ← Pen features
├── girobot_step6.md              ← Geometry & text
├── girobot_step7.md              ← WebSocket backend
├── girobot_step8.md              ← Web frontend
├── girobot_step9.md              ← Web controls
├── girobot_step10.md             ← Map display
├── girobot_step11.md             ← Battery fix
├── girobot_step12.md             ← Integration testing
├── girobot_step13.md             ← Bluetooth AI Debug Bridge
│
├── platformio.ini                ← Build config
├── include/
│   ├── Config.h                  ← Pin definitions & constants
│   ├── DCMotor.h                 ← Motor control interface
│   ├── DriveTrain.h              ← Differential drive
│   ├── Navigation.h              ← IMU sensor fusion
│   ├── PoseEstimator.h           ← Odometry + IMU
│   ├── MotionController.h        ← Path following (PID)
│   ├── PathPlanner.h             ← Geometry & text rendering
│   ├── CommsManager.h            ← WiFi & WebSocket
│   ├── BluetoothManager.h        ← BLE Debug Interface
│   ├── ConfigManager.h           ← Parameter storage (NVS)
│   ├── BatteryMonitor.h          ← Battery ADC
│   └── ...
│
├── src/
│   ├── main.cpp                  ← System orchestration
│   ├── DCMotor.cpp
│   ├── DriveTrain.cpp
│   ├── Navigation.cpp
│   ├── PoseEstimator.cpp
│   ├── MotionController.cpp      ← Needs PID enhancement
│   ├── PathPlanner.cpp
│   ├── CommsManager.cpp
│   ├── BluetoothManager.cpp
│   ├── ConfigManager.cpp
│   ├── BatteryMonitor.cpp        ← Needs ADC fix
│   └── ...
│
└── data/
    ├── index.html                ← Web dashboard (CREATE)
    ├── style.css                 ← Styling (CREATE)
    └── script.js                 ← WebSocket client (CREATE)
```

---

## ✅ Verification Strategy

Each phase includes specific checks:
- **Serial output validation**: Motor steps, sensor readings, odometry drift
- **Hardware tests**: Motor speed ramps, encoder counts, IMU calibration
- **Motion accuracy**: Straight-line drift, rotation precision, waypoint following
- **Web interface**: WebSocket connectivity, command execution, telemetry display
- **Integration**: Full system test with geometry drawing and text rendering

---

## 🎓 Key Algorithms & Concepts

### 1. **Differential Drive Kinematics**
Two independent rear wheels; pen offset from axle creates complex trajectory geometry.

```
V_left  = V_linear - (ω × L/2)     [wheel base compensation]
V_right = V_linear + (ω × L/2)
```

### 2. **IMU Sensor Fusion**
Mahony filter combines gyro, accel, and magnetometer for stable orientation.

```
Heading = α × Gyro_integrated + (1-α) × Mag_absolute
where α ≈ 0.98 (blending factor, tuned per hardware)
```

### 3. **Odometry + IMU Fusion for Position**
Encoder odometry provides accurate distance; IMU heading fusion corrects gyro drift.

```
ΔX = Δsteps × (π × wheel_diameter) / steps_per_rev × cos(heading)
ΔY = Δsteps × (π × wheel_diameter) / steps_per_rev × sin(heading)
```

### 4. **PID Path Following**
Cross-track error feedback with proportional, integral, and derivative terms.

```
Steering_command = Kp × error + Ki × ∫error + Kd × d(error)/dt
```

### 5. **Pen-Centric Trajectory Correction**
Pen tip is 130mm ahead of axle; offset must be applied to path planning.

```
Axle_trajectory = required_pen_trajectory + offset_vector(heading, 130mm)
```

---

## 🛠️ Tools & Dependencies

**Firmware Development:**
- PlatformIO (with ESP32 toolchain)
- Arduino framework
- Libraries: ESP Async WebServer, ArduinoJson, AccelStepper, Bolder Flight Systems MPU9250

**Web Development:**
- HTML5 Canvas for real-time robot visualization
- WebSocket API for bidirectional communication
- CSS3 for responsive layout

**Hardware Testing:**
- Serial monitor (115200 baud)
- Multimeter (battery voltage verification)
- Ruler/measuring tape (motion accuracy)

---

## 📝 Notes

- **Battery Issue**: ADC pin 0 is not working (Phase 11 diagnosis required). May need GPIO36 or GPIO39 alternative.
- **PID Enhancement**: MotionController.cpp needs full Ki/Kd integration; currently only Kp visible.
- **Simulation Mode**: Entire stack supports VirtualStepper (no hardware); useful for desktop testing and debugging.
- **Auto-Detection**: Navigation.cpp automatically detects MPU9250 (0x1E) or BNO055 (0x6B) via I2C scan.

---

## 🔄 Recommended Workflow

1. **Parallel Development**:
   - Phases 0, 7, 11 can start immediately (no hardware dependencies)
   - Phases 1, 2 require hardware (motors, IMU)
   - Phases 3, 4, 5, 6 depend on 1 & 2
   - Phases 8, 9, 10 depend on 7

2. **Testing Strategy**:
   - Phase 0: Build verification
   - Phase 1-2: Hardware self-test (SelfTest.cpp)
   - Phase 3: Odometry validation (stationary drift check)
   - Phase 4: Motion accuracy (straight line, turn in place)
   - Phase 5-6: Drawing accuracy
   - Phase 7: WebSocket connectivity
   - Phase 8-10: Web UI responsive, telemetry updates
   - Phase 12: Full system integration

---

## 📉 AI Token Optimization

To minimize token usage and maximize the efficiency of the AI coding assistant:

1. **Modular Headers**: When asking about architecture, provide only `.h` files. Provide `.cpp` files only when debugging specific logic.
2. **Protocol Compression**: Use the "Short-Key" JSON format (`{"c":"S"}` vs `{"cmd":"STOP"}`) to keep telemetry definitions compact.
3. **Context Pruning**: Remove verbose library comments, boilerplate license headers, and excessive log strings before sharing code with the AI.
4. **Incremental Context**: Focus on one module or one specific bug at a time rather than providing the entire codebase.
5. **Error Summarization**: Instead of pasting full serial logs, provide the specific error message and the 10 lines of code immediately surrounding the crash point.
6. **Type Hinting**: Keep `Config.h` updated so the AI knows the exact pin and constant values without searching multiple files.

---

## 📚 References

- **[girobot_step0.md](girobot_step0.md)**: Start here → Project structure & build verification
- **[girobot_step1.md](girobot_step1.md)**: Motor control & encoder feedback
- **[girobot_step2.md](girobot_step2.md)**: IMU sensor fusion & calibration
- **[girobot_step3.md](girobot_step3.md)**: Position tracking & odometry
- **[girobot_step4.md](girobot_step4.md)**: Motion commands & PID tuning
- **[girobot_step5.md](girobot_step5.md)**: Pen features & drawing
- **[girobot_step6.md](girobot_step6.md)**: Geometry & text rendering
- **[girobot_step7.md](girobot_step7.md)**: WebSocket backend
- **[girobot_step8.md](girobot_step8.md)**: Web dashboard creation
- **[girobot_step9.md](girobot_step9.md)**: Web controls & commands
- **[girobot_step10.md](girobot_step10.md)**: Map & ghost visualization
- **[girobot_step11.md](girobot_step11.md)**: Battery diagnostics
- **[girobot_step12.md](girobot_step12.md)**: Full integration test

---

**Last Updated**: May 11, 2026  
**Status**: Implementation Plan Complete - Ready for Phase 0 ✅
