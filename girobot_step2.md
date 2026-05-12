# GiRobot Step 2: IMU Sensor Integration

> **Phase 2**: Initialize MPU9250/BNO055 sensor, calibrate all axes (gyro, accel, magnetometer), implement sensor fusion, and verify heading stability.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand I2C communication with MPU9250 (9-axis) or BNO055 (alternative 9-axis)
2. ✅ Auto-detect IMU sensor and read raw data (gyro, accel, mag)
3. ✅ Calibrate gyroscope zero-rate offset (stationary baseline)
4. ✅ Calibrate accelerometer level offset (gravity vector)
5. ✅ Calibrate magnetometer hard-iron and soft-iron distortion
6. ✅ Implement Mahony sensor fusion filter
7. ✅ Verify heading accuracy (< 5°/min drift when stationary)
8. ✅ Test yaw tracking during rotation

---

## 🔌 Hardware Wiring Reference

From [include/Config.h](../include/Config.h):

```cpp
constexpr int PinI2cSda = 21;      // I2C Serial Data
constexpr int PinI2cScl = 22;      // I2C Serial Clock
constexpr int ImuAddress = 0;      // Auto-detect (0 = scan I2C bus)
```

### I2C Connections

| GPIO | Signal | IMU Pin | Notes |
|------|--------|---------|-------|
| 21 | SDA | SDA | Serial data (pull-up 4.7k typical) |
| 22 | SCL | SCL | Serial clock (pull-up 4.7k typical) |
| 3.3V | VCC | VIN | Power supply |
| GND | GND | GND | Common ground |

### MPU9250 I2C Addresses
- **Default**: 0x68 (AD0 pin grounded)
- **Alternate**: 0x69 (AD0 pin high)
- **AutoIMU.cpp scans** for devices at both addresses

### BNO055 I2C Addresses
- **Default**: 0x28 (COM3 grounded)
- **Alternate**: 0x29 (COM3 high)

---

## 📖 Understanding Navigation.cpp

Review [src/Navigation.cpp](../src/Navigation.cpp) to understand:

### 1. **Sensor Fusion Algorithm: Mahony Filter**

The Mahony filter combines three sensor types to produce stable orientation:

```
Input:  Gyroscope (angular rate)  ← Fast, accurate, drifts over time
        Accelerometer (gravity)   ← Slow, noisy, no drift
        Magnetometer (B-field)    ← Slow, noisy, affected by distortions

Output: Stable heading (yaw)
        Pitch & roll (optional)
        Orientation quaternion
```

**How it works**:
1. **Gyroscope integration**: Rapidly integrate angular rates to get orientation
2. **Error detection**: Measure difference between gyro-based and accel/mag-based heading
3. **Correction**: Feed error back to gyro bias estimates
4. **Result**: Gyro speed + accel/mag accuracy

**Pseudocode**:
```
while (running) {
    // Read sensors
    gyro_rate = readGyroscope();        // deg/s
    accel = readAccelerometer();        // m/s²
    mag = readMagnetometer();           // µT
    
    // Gyro integration (fast)
    heading_gyro += gyro_rate * dt;
    
    // Accel-based heading (slow)
    roll = atan2(accel.y, accel.z);
    pitch = atan2(-accel.x, sqrt(accel.y² + accel.z²));
    
    // Mag-based heading (slow, compensated for tilt)
    heading_mag = atan2(mag_corrected.y, mag_corrected.x);
    
    // Blend (complementary filter)
    // α = 0.98: favor gyro (fast), 0.02: correction from accel/mag
    heading_final = 0.98 * heading_gyro + 0.02 * heading_mag;
    
    // Update gyro bias estimate
    gyro_bias += Ki * (heading_mag - heading_gyro) * dt;
}
```

### 2. **Calibration Data Structure**

All calibration offsets stored in `ConfigManager` (persistent in NVS):

```cpp
// Gyroscope calibration (zero-rate offset)
float gyroOffsetX, gyroOffsetY, gyroOffsetZ;    // deg/s

// Accelerometer calibration (gravity offset)
float accelOffsetX, accelOffsetY, accelOffsetZ; // m/s²

// Magnetometer calibration (hard-iron distortion)
float magOffsetX, magOffsetY, magOffsetZ;       // µT

// Magnetometer scaling (soft-iron distortion)
float magScaleX, magScaleY, magScaleZ;          // Unitless
```

### 3. **Key Navigation Methods**

#### begin()
Auto-detect IMU and initialize I2C:
```cpp
void Navigation::begin() {
    // Initialize I2C bus
    Wire.begin(PinI2cSda, PinI2cScl);
    
    // Scan for IMU
    if (!autoDetectIMU()) {
        Serial.println("[ERROR] IMU not found!");
        return;
    }
    
    // Read calibration from NVS
    loadCalibration();
    
    // Configure sensor data rates
    configureSensorRates();
}
```

#### update()
Call this in main loop (~100+ Hz) to read sensors and fuse data:
```cpp
void Navigation::update() {
    // Read raw sensor data
    readSensors();
    
    // Apply calibration offsets
    applyCalibration();
    
    // Run Mahony filter
    updateMahonyFilter(dt);
    
    // Extract heading, pitch, roll from quaternion
    extractEulerAngles();
}
```

#### Calibration Methods

**Gyroscope Calibration**:
```cpp
void Navigation::calibrateGyro() {
    Serial.println("[CALIB] Gyro calibration: Keep robot STILL for 5 seconds...");
    
    float sumX=0, sumY=0, sumZ=0;
    int samples = 0;
    
    uint32_t startTime = millis();
    while (millis() - startTime < 5000) {
        readSensors();
        sumX += gyroX;
        sumY += gyroY;
        sumZ += gyroZ;
        samples++;
        delay(10);
    }
    
    // Average = offset
    gyroOffsetX = sumX / samples;
    gyroOffsetY = sumY / samples;
    gyroOffsetZ = sumZ / samples;
    
    Serial.printf("[CALIB] Gyro offset: (%.2f, %.2f, %.2f) deg/s\n",
                  gyroOffsetX, gyroOffsetY, gyroOffsetZ);
    config.save();  // Persist to NVS
}
```

**Magnetometer Calibration**:
```cpp
void Navigation::calibrateMagnetometer() {
    Serial.println("[CALIB] Mag calibration: Rotate robot 360° SLOWLY for 15 seconds...");
    
    float minX=9999, maxX=-9999;
    float minY=9999, maxY=-9999;
    float minZ=9999, maxZ=-9999;
    
    uint32_t startTime = millis();
    while (millis() - startTime < 15000) {
        readSensors();
        
        minX = min(minX, magX);  maxX = max(maxX, magX);
        minY = min(minY, magY);  maxY = max(maxY, magY);
        minZ = min(minZ, magZ);  maxZ = max(maxZ, magZ);
        
        Serial.print(".");
        delay(50);
    }
    
    // Hard-iron offset (center of min/max)
    magOffsetX = (maxX + minX) / 2.0f;
    magOffsetY = (maxY + minY) / 2.0f;
    magOffsetZ = (maxZ + minZ) / 2.0f;
    
    // Soft-iron scaling (normalize ranges)
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;
    
    float maxRange = max({rangeX, rangeY, rangeZ});
    magScaleX = maxRange / rangeX;
    magScaleY = maxRange / rangeY;
    magScaleZ = maxRange / rangeZ;
    
    Serial.printf("\n[CALIB] Mag offsets: (%.1f, %.1f, %.1f) µT\n",
                  magOffsetX, magOffsetY, magOffsetZ);
    Serial.printf("[CALIB] Mag scales: (%.3f, %.3f, %.3f)\n",
                  magScaleX, magScaleY, magScaleZ);
    
    config.save();  // Persist to NVS
}
```

---

## 🧪 Testing Procedure

### Test 1: IMU Detection & Boot

**Expected Behavior**: Serial output shows IMU detected at correct address.

**Steps**:
1. Power on ESP32
2. Open serial monitor (115200 baud)
3. Look for I2C detection output

**Expected Serial Output**:
```
[I] Initializing I2C bus (SDA=GPIO21, SCL=GPIO22)...
[I] Scanning I2C bus for devices...
[I] Found device at address 0x1E (MPU9250)
[I] Configuring MPU9250...
[I] IMU initialization complete
[I] System ready!
```

**Troubleshooting I2C Not Found**:
- [ ] Check GPIO 21 (SDA) and GPIO 22 (SCL) wiring
- [ ] Verify pull-up resistors (4.7k typical) on both lines
- [ ] Confirm sensor power supply (3.3V)
- [ ] Check I2C address with scanner:
  ```cpp
  // Add temporary I2C scan code
  Wire.begin(21, 22);
  for (byte i=0; i<127; i++) {
      Wire.beginTransmission(i);
      if (Wire.endTransmission() == 0) {
          Serial.printf("Device found at address 0x%02X\n", i);
      }
  }
  ```

---

### Test 2: Raw Sensor Data Output

**Expected Behavior**: Serial shows stable gyro/accel/mag readings (no NaN, no extreme spikes).

**Add to serial monitor output**:

```cpp
// In main loop, add telemetry:
uint32_t lastTelemetry = 0;
if (millis() - lastTelemetry > 200) {  // Every 200ms
    lastTelemetry = millis();
    
    Serial.printf("[IMU] Gyro: (%.2f, %.2f, %.2f) deg/s\n",
                  nav.getGyroX(), nav.getGyroY(), nav.getGyroZ());
    Serial.printf("[IMU] Accel: (%.2f, %.2f, %.2f) m/s²\n",
                  nav.getAccelX(), nav.getAccelY(), nav.getAccelZ());
    Serial.printf("[IMU] Mag: (%.1f, %.1f, %.1f) µT\n",
                  nav.getMagX(), nav.getMagY(), nav.getMagZ());
}
```

**Expected Values**:
```
[IMU] Gyro: (0.12, -0.08, 0.05) deg/s       ← Small values (sensor noise)
[IMU] Accel: (0.15, -0.20, 9.81) m/s²      ← Z≈9.81 (gravity), X/Y small
[IMU] Mag: (23.4, -18.2, 45.1) µT          ← Depends on magnetic field
```

**Red Flags**:
- [ ] Gyro > 10 deg/s while stationary → Check sensor orientation
- [ ] Accel Z not ≈ 9.81 m/s² on flat surface → Sensor tilted
- [ ] Any value = NaN → I2C communication error or sensor fault
- [ ] Magnetometer extreme values → Strong local magnetic distortion

---

### Test 3: Gyroscope Calibration

**Expected Behavior**: Gyro offsets measured and stored; removes systematic bias.

**Procedure**:
1. Place robot on FLAT, LEVEL surface
2. Keep completely STILL (no vibrations)
3. Run gyro calibration:

```cpp
// Add to main or test function:
Serial.println("\n[INIT] Starting gyroscope calibration...");
nav.calibrateGyro();
Serial.println("[OK] Gyro calibration complete.");
```

**Expected Serial Output**:
```
[CALIB] Gyro calibration: Keep robot STILL for 5 seconds...
[CALIB] Gyro offset: (-0.45, 0.32, -0.18) deg/s
[OK] Calibration saved to NVS
```

**Verification**:
After calibration, stationary gyro readings should be near 0:
```
[IMU] Gyro: (0.02, 0.01, -0.03) deg/s    ← All near 0
```

---

### Test 4: Accelerometer Verification

**Expected Behavior**: Verify accelerometer correctly detects gravity (9.81 m/s²).

**Procedure**:
1. Place robot on FLAT surface
2. Check serial output for accelerometer Z value

**Expected Output**:
```
[IMU] Accel: (0.05, -0.10, 9.81) m/s²   ← Z = gravity when level
```

**If Z ≠ 9.81**:
- [ ] Robot tilted or sensor not level
- [ ] Sensor offset calibration needed
- [ ] Check sensor datasheet for scale factor

---

### Test 5: Magnetometer Calibration

**Expected Behavior**: Captures magnetic distortions from current environment.

**Procedure**:
1. Clear workspace of metal objects
2. Place robot on flat surface
3. Run magnetometer calibration:

```cpp
Serial.println("\n[INIT] Starting magnetometer calibration...");
nav.calibrateMagnetometer();
Serial.println("[OK] Mag calibration complete.");
```

**During calibration** (15 seconds):
- Slowly rotate robot 360° (one full spin)
- Rotate smoothly (avoid jerky movements)
- Keep robot on same plane (don't tilt)

**Expected Serial Output**:
```
[CALIB] Mag calibration: Rotate robot 360° SLOWLY for 15 seconds...
........................... [dots every 50ms]
[CALIB] Mag offsets: (15.2, -8.5, 3.1) µT
[CALIB] Mag scales: (1.025, 0.998, 1.007)
[OK] Calibration saved to NVS
```

**Interpretation**:
- **Offsets**: Remove hard-iron distortion (permanent magnets nearby)
- **Scales**: Correct soft-iron distortion (material permeability)
- Scales near 1.0 (0.98-1.02) = good calibration
- Scales > 1.05 = significant distortion, might need relocation

---

### Test 6: Heading Stability (Stationary)

**Expected Behavior**: Yaw angle drifts < 5°/min when robot is stationary.

**Procedure**:

```cpp
void testHeadingStability() {
    Serial.println("\n[TEST] Heading stability test (30 seconds)...");
    Serial.println("Robot must remain STILL and LEVEL.");
    
    float initialHeading = nav.getYaw();
    uint32_t startTime = millis();
    float maxDrift = 0;
    
    while (millis() - startTime < 30000) {
        nav.update();
        
        float currentHeading = nav.getYaw();
        float drift = abs(currentHeading - initialHeading);
        
        if (drift > maxDrift) {
            maxDrift = drift;
        }
        
        if ((millis() - startTime) % 5000 == 0) {
            float driftPerMinute = (drift / (millis() - startTime)) * 60000;
            Serial.printf("[DRIFT] Time=%ldms, Heading=%.1f°, Drift=%.1f° (%.1f°/min)\n",
                          millis() - startTime,
                          currentHeading,
                          drift,
                          driftPerMinute);
        }
        
        delay(10);
    }
    
    float finalDriftPerMinute = (maxDrift / 30000.0) * 60000.0;
    Serial.printf("\n[RESULT] Max drift over 30s: %.1f° (%.1f°/min)\n",
                  maxDrift, finalDriftPerMinute);
    
    if (finalDriftPerMinute < 5.0) {
        Serial.println("[PASS] Heading stability excellent!");
    } else if (finalDriftPerMinute < 10.0) {
        Serial.println("[WARN] Heading stability acceptable (minor gyro drift)");
    } else {
        Serial.println("[FAIL] Excessive heading drift - check calibration");
    }
}
```

**Expected Output**:
```
[TEST] Heading stability test (30 seconds)...
[DRIFT] Time=5000ms, Heading=0.2°, Drift=0.2° (0.24°/min)
[DRIFT] Time=10000ms, Heading=0.1°, Drift=0.1° (0.06°/min)
[DRIFT] Time=15000ms, Heading=-0.3°, Drift=0.3° (0.12°/min)
[DRIFT] Time=20000ms, Heading=0.4°, Drift=0.4° (0.12°/min)
[DRIFT] Time=25000ms, Heading=-0.1°, Drift=0.1° (0.02°/min)
[DRIFT] Time=30000ms, Heading=0.2°, Drift=0.2° (0.04°/min)

[RESULT] Max drift over 30s: 0.4° (0.48°/min)
[PASS] Heading stability excellent!
```

**Pass Criteria**: < 5°/min drift when stationary

---

### Test 7: Heading Tracking During Rotation

**Expected Behavior**: Yaw angle changes match actual rotation (≈90° per quarter turn).

**Procedure**:

```cpp
void testHeadingTracking() {
    Serial.println("\n[TEST] Heading tracking during rotation...");
    
    float headingAtStart = nav.getYaw();
    Serial.printf("Starting heading: %.1f°\n", headingAtStart);
    
    Serial.println("Slowly rotate robot 90° clockwise (15 seconds)...");
    delay(2000);
    
    uint32_t startTime = millis();
    while (millis() - startTime < 15000) {
        nav.update();
        
        if ((millis() - startTime) % 3000 == 0) {
            float currentHeading = nav.getYaw();
            Serial.printf("  Time=%ldms, Heading=%.1f°\n",
                          millis() - startTime, currentHeading);
        }
        
        delay(10);
    }
    
    float headingAtEnd = nav.getYaw();
    float headingChange = headingAtEnd - headingAtStart;
    
    Serial.printf("Final heading: %.1f°, Change: %.1f°\n",
                  headingAtEnd, headingChange);
    
    // Account for heading wrapping (359° -> 1° = 2° change, not -358°)
    if (headingChange < -180) headingChange += 360;
    if (headingChange > 180) headingChange -= 360;
    
    Serial.printf("Heading change (corrected): %.1f° (expected: ±90°)\n", headingChange);
    
    if (abs(headingChange - 90.0) < 10.0 || abs(headingChange + 90.0) < 10.0) {
        Serial.println("[PASS] Heading tracking accurate!");
    } else {
        Serial.println("[WARN] Heading change unexpectedCheck calibration or rotation speed");
    }
}
```

**Expected Output**:
```
[TEST] Heading tracking during rotation...
Starting heading: 15.2°
Slowly rotate robot 90° clockwise (15 seconds)...
  Time=3000ms, Heading=30.1°
  Time=6000ms, Heading=47.5°
  Time=9000ms, Heading=68.3°
  Time=12000ms, Heading=92.8°
  Time=15000ms, Heading=107.3°
Final heading: 107.3°, Change: 92.1°
Heading change (corrected): 92.1° (expected: ±90°)
[PASS] Heading tracking accurate!
```

**Pass Criteria**: Heading change within ±10° of commanded rotation

---

## ✅ Verification Checklist: Step 2

- [ ] **IMU Detection**: Serial shows sensor found at I2C address
- [ ] **Raw Sensor Data**: Gyro, accel, mag values displayed without NaN
- [ ] **Gyro Calibration**: Offsets measured and saved to NVS
- [ ] **Accel Verification**: Z ≈ 9.81 m/s² on level surface
- [ ] **Mag Calibration**: Offsets and scales measured by rotating 360°
- [ ] **Heading Stability**: Drift < 5°/min when stationary
- [ ] **Heading Tracking**: Yaw change ≈ ±90° during quarter-turn rotation
- [ ] **No NaN Values**: All sensor readings are valid numbers

---

## 🐛 Troubleshooting

### I2C Bus Not Responding
**Check**:
- [ ] Wiring: GPIO 21 (SDA), GPIO 22 (SCL)
- [ ] Pull-up resistors on SDA/SCL (4.7k typical)
- [ ] Sensor power supply (3.3V)
- [ ] Use I2C scanner to find device address

### Magnetometer Unstable / Extreme Values
**Cause**: Local magnetic distortion (metal objects, electronics)

**Solution**:
- [ ] Move robot away from sources (metal shelves, power supplies)
- [ ] Redo magnetometer calibration in final location
- [ ] If still problematic, may need to add ferrite shielding

### Gyro Drifts > 10°/min
**Cause**: Calibration incomplete or sensor temperature effects

**Solution**:
- [ ] Re-run gyro calibration (ensure complete stillness)
- [ ] Let sensor warm up 5 minutes before calibrating
- [ ] Check if sensor mounted on vibrating surface (dampen with foam)

### Heading Jumps Unexpectedly
**Cause**: Magnetometer reading sudden distortion

**Solution**:
- [ ] Check for magnetic sources turning on/off nearby
- [ ] Reduce magnetometer measurement weight (increase blending factor α)
- [ ] Consider disabling magnetometer indoors; use pure gyro integration

---

## 📊 Sample Serial Output (Successful Phase 2)

```
[I] Firmware Version: 0.1.1
[I] System Mode: REAL
[I] Initializing I2C bus (SDA=GPIO21, SCL=GPIO22)...
[I] Scanning I2C bus for devices...
[I] Found device at address 0x1E (MPU9250)
[I] Configuring MPU9250...
[I] Gyroscope range: ±250 deg/s
[I] Accelerometer range: ±2g
[I] Magnetometer range: ±4900 µT
[I] Sensor data rate: 200 Hz
[I] IMU initialization complete
[I] System ready!

--- IMU Sensor Readings (raw) ---
[IMU] Gyro: (0.12, -0.08, 0.05) deg/s
[IMU] Accel: (0.15, -0.20, 9.81) m/s²
[IMU] Mag: (23.4, -18.2, 45.1) µT

--- Starting Calibration Sequence ---

[INIT] Starting gyroscope calibration...
[CALIB] Gyro calibration: Keep robot STILL for 5 seconds...
[CALIB] Gyro offset: (-0.45, 0.32, -0.18) deg/s
[OK] Gyro calibration complete.

[INIT] Starting magnetometer calibration...
[CALIB] Mag calibration: Rotate robot 360° SLOWLY for 15 seconds...
...........................
[CALIB] Mag offsets: (15.2, -8.5, 3.1) µT
[CALIB] Mag scales: (1.025, 0.998, 1.007)
[OK] Mag calibration complete.

--- Heading Stability Test ---
[TEST] Heading stability test (30 seconds)...
[DRIFT] Time=5000ms, Heading=0.2°, Drift=0.2° (0.24°/min)
[DRIFT] Time=10000ms, Heading=0.1°, Drift=0.1° (0.06°/min)
[DRIFT] Time=30000ms, Heading=0.2°, Drift=0.2° (0.04°/min)
[RESULT] Max drift over 30s: 0.4° (0.48°/min)
[PASS] Heading stability excellent!

--- Heading Tracking Test ---
[TEST] Heading tracking during rotation...
Starting heading: 15.2°
Slowly rotate robot 90° clockwise (15 seconds)...
  Time=3000ms, Heading=30.1°
  Time=6000ms, Heading=47.5°
  Time=12000ms, Heading=92.8°
Final heading: 107.3°, Change: 92.1°
Heading change (corrected): 92.1° (expected: ±90°)
[PASS] Heading tracking accurate!

--- All IMU Tests Passed! ---
```

---

## 📝 Notes & Observations

1. **Sensor Fusion Tuning**:
   - Blending factor α = 0.98 (favor gyro)
   - Ki = 0.002 (integral windup constant)
   - Adjust based on drift behavior

2. **Magnetometer Limitations**:
   - Magnetic fields vary by location
   - Indoor environments have distortions
   - Calibration is location-specific

3. **Temperature Drift**:
   - Sensors drift with temperature changes
   - Gyro sensitive to board heating
   - Re-calibrate if temperature changes > 10°C

4. **Complementary vs Mahony**:
   - Mahony: Better for high-dynamic motion
   - Complementary: Simpler, sufficient for slow drawing robot
   - Current implementation uses Mahony filter

---

## 🎯 Next Steps

**Phase 2 Complete!**
→ Move to [girobot_step3.md](girobot_step3.md): **Odometry & Position Tracking**

In Phase 3, you will:
1. Combine encoder odometry with IMU heading
2. Track pen position (X, Y, θ) in real-time
3. Verify position accuracy over 1-meter path
4. Implement ghost pose tracking (pure odometry)

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate (sensor calibration)  
**Hardware Required**: ESP32, MPU9250 or BNO055, I2C pull-up resistors  

Last Updated: May 11, 2026
