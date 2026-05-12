# GiRobot Step 3: Odometry & Position Tracking

> **Phase 3**: Fuse encoder odometry with IMU heading to estimate pen position (X, Y, θ) in real-time and verify accuracy.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand odometry calculation (wheel encoder → distance → position)
2. ✅ Fuse encoder odometry with IMU heading for stable position estimate
3. ✅ Verify position accuracy over known distances (5m test path)
4. ✅ Implement ghost pose tracking (pure odometry for drift visualization)
5. ✅ Test heading stability during straight-line motion
6. ✅ Measure accumulated error over time

---

## 🔧 Core Concepts

### 1. **Encoder Odometry**

Convert wheel encoder counts to traveled distance:

```
Steps_traveled = Encoder_count
Distance_mm = Steps_traveled × (π × WheelDiameter) / StepsPerRev
Distance_mm = Steps_traveled × STEPS_PER_MM

From Config.h:
  WheelDiameter = 90 mm
  StepsPerRev = 1070 steps
  STEPS_PER_MM = 1070 / (π × 90) ≈ 3.76 steps/mm
```

### 2. **Differential Drive Kinematics**

Two independent wheels create position and heading:

```
ΔX = (Distance_left + Distance_right) / 2 × cos(heading)
ΔY = (Distance_left + Distance_right) / 2 × sin(heading)
Δθ = (Distance_right - Distance_left) / WheelBase

where:
  Distance_left = steps_left / STEPS_PER_MM
  Distance_right = steps_right / STEPS_PER_MM
  WheelBase = 83 mm (from Config.h)
  heading = current IMU yaw angle (degrees → radians)
```

### 3. **Pen-Centric Coordinate System**

The pen tip is the origin (0,0). The axle center is offset:

```
Pen position = Axle position + offset_vector
offset_vector = [130 mm × cos(heading), 130 mm × sin(heading)]

where 130 mm = PenOffsetMm from Config.h
```

### 4. **IMU Heading Fusion**

Short-term: Gyro integration (fast, drifts)  
Long-term: Magnetometer absolute heading (slow, stable)

```
Blended heading = 0.98 × gyro_heading + 0.02 × mag_heading
```

### 5. **Ghost Pose Tracking**

Pure odometry (encoder-only) for comparison:

```
Ghost_pose = odometry_only (no IMU heading correction)
Real_pose = odometry + IMU_fusion

Drift_visualization = Real_pose - Ghost_pose
```

---

## 📖 Understanding PoseEstimator.cpp

Review [src/PoseEstimator.cpp](../src/PoseEstimator.cpp):

### Key Methods

#### begin()
Initialize position at origin:
```cpp
void PoseEstimator::begin() {
    penX = 0.0f;
    penY = 0.0f;
    heading = 0.0f;         // radians
    
    // Get baseline encoder counts
    lastLeftSteps = drive.getLeftMotor().getCurrentPosition();
    lastRightSteps = drive.getRightMotor().getCurrentPosition();
}
```

#### update()
Called every main loop iteration (~100 Hz):

```cpp
void PoseEstimator::update() {
    // Get new encoder counts
    long leftSteps = drive.getLeftMotor().getCurrentPosition();
    long rightSteps = drive.getRightMotor().getCurrentPosition();
    
    // Calculate delta steps
    long deltaLeftSteps = leftSteps - lastLeftSteps;
    long deltaRightSteps = rightSteps - lastRightSteps;
    
    // Convert steps to mm
    float deltaLeftMm = deltaLeftSteps / STEPS_PER_MM;
    float deltaRightMm = deltaRightSteps / STEPS_PER_MM;
    
    // Average distance traveled
    float avgDistanceMm = (deltaLeftMm + deltaRightMm) / 2.0f;
    
    // Get heading from IMU
    heading = nav.getYaw() * PI / 180.0f;  // Convert degrees to radians
    
    // Update position
    penX += avgDistanceMm * cos(heading);
    penY += avgDistanceMm * sin(heading);
    
    // Also track heading from odometry only (for ghost pose)
    float deltaHeadingOdometry = (deltaRightMm - deltaLeftMm) / WHEEL_BASE;
    ghostHeading += deltaHeadingOdometry;
    ghostX += avgDistanceMm * cos(ghostHeading);
    ghostY += avgDistanceMm * sin(ghostHeading);
    
    // Update baseline for next iteration
    lastLeftSteps = leftSteps;
    lastRightSteps = rightSteps;
}
```

#### Accessor Methods
```cpp
float getPenX() { return penX; }
float getPenY() { return penY; }
float getHeading() { return heading; }
float getAxleCenterX() { return penX - PEN_OFFSET_MM * cos(heading); }
float getAxleCenterY() { return penY - PEN_OFFSET_MM * sin(heading); }
float getGhostX() { return ghostX; }
float getGhostY() { return ghostY; }
```

---

## 🧪 Testing Procedure

### Test 1: Static Position (No Motion)

**Expected**: Position remains at origin (0, 0) when motors are off.

```cpp
void testStaticPosition() {
    Serial.println("\n[TEST] Static position test (10 seconds)...");
    
    // Reset position
    pose.reset();
    
    uint32_t startTime = millis();
    while (millis() - startTime < 10000) {
        pose.update();
        
        if ((millis() - startTime) % 2000 == 0) {
            Serial.printf("[STATIC] X=%.1f mm, Y=%.1f mm, θ=%.1f°\n",
                          pose.getPenX(), pose.getPenY(),
                          pose.getHeading() * 180 / PI);
        }
        delay(10);
    }
    
    Serial.printf("[RESULT] Final position: (%.1f, %.1f) mm\n",
                  pose.getPenX(), pose.getPenY());
}
```

**Expected Output**:
```
[TEST] Static position test (10 seconds)...
[STATIC] X=0.0 mm, Y=0.0 mm, θ=0.0°
[STATIC] X=0.0 mm, Y=0.0 mm, θ=0.0°
[STATIC] X=0.0 mm, Y=0.0 mm, θ=0.0°
[STATIC] X=0.0 mm, Y=0.0 mm, θ=0.0°
[STATIC] X=0.0 mm, Y=0.0 mm, θ=0.0°
[RESULT] Final position: (0.0, 0.0) mm
[PASS] No drift detected
```

---

### Test 2: Straight-Line Accuracy (200 mm)

**Expected**: Robot travels 200 mm forward; pen position Y ≈ 200 mm, X ≈ 0 mm.

**Procedure**:
1. Mark starting position with pen
2. Run test
3. Mark ending position
4. Measure actual distance with ruler

```cpp
void testStraightLine() {
    Serial.println("\n[TEST] Straight-line test (200 mm)...");
    Serial.println("Robot will move forward. Mark start/end with pen.");
    
    pose.reset();
    
    // Move forward at 50 mm/s for 4 seconds (200 mm)
    drive.setLinearVelocity(50.0f);  // mm/s
    drive.setAngularVelocity(0.0f);  // rad/s
    
    uint32_t startTime = millis();
    while (millis() - startTime < 4000) {
        pose.update();
        
        if ((millis() - startTime) % 500 == 0) {
            float distanceTraveled = sqrt(pose.getPenX()*pose.getPenX() + 
                                          pose.getPenY()*pose.getPenY());
            Serial.printf("[STRAIGHT] Time=%ldms, X=%.1f mm, Y=%.1f mm, Distance=%.1f mm\n",
                          millis() - startTime,
                          pose.getPenX(), pose.getPenY(), distanceTraveled);
        }
        delay(10);
    }
    
    // Stop
    drive.stop();
    
    float finalX = pose.getPenX();
    float finalY = pose.getPenY();
    float lateralError = abs(finalX);  // Should be ~0 for straight line
    float distanceError = abs(finalY - 200.0f);
    
    Serial.printf("[RESULT] Final: X=%.1f mm, Y=%.1f mm\n", finalX, finalY);
    Serial.printf("[ERROR] Lateral: %.1f mm (target 0), Distance: %.1f mm (target 200)\n",
                  lateralError, distanceError);
    
    if (lateralError < 5.0 && distanceError < 10.0) {
        Serial.println("[PASS] Straight-line accuracy excellent!");
    } else if (lateralError < 15.0 && distanceError < 20.0) {
        Serial.println("[WARN] Acceptable accuracy, but may need motor tuning");
    } else {
        Serial.println("[FAIL] Significant error - check encoder calibration");
    }
}
```

**Expected Output**:
```
[TEST] Straight-line test (200 mm)...
[STRAIGHT] Time=500ms, X=0.2 mm, Y=25.1 mm, Distance=25.1 mm
[STRAIGHT] Time=1000ms, X=0.1 mm, Y=50.3 mm, Distance=50.3 mm
[STRAIGHT] Time=1500ms, X=-0.3 mm, Y=75.2 mm, Distance=75.2 mm
[STRAIGHT] Time=2000ms, X=0.4 mm, Y=100.1 mm, Distance=100.1 mm
[STRAIGHT] Time=2500ms, X=-0.1 mm, Y=125.0 mm, Distance=125.0 mm
[STRAIGHT] Time=3000ms, X=0.2 mm, Y=150.2 mm, Distance=150.2 mm
[STRAIGHT] Time=3500ms, X=0.3 mm, Y=175.1 mm, Distance=175.1 mm
[RESULT] Final: X=0.5 mm, Y=200.2 mm
[ERROR] Lateral: 0.5 mm (target 0), Distance: 0.2 mm (target 200)
[PASS] Straight-line accuracy excellent!
```

**Pass Criteria**:
- Lateral error (X) < 5 mm
- Distance error (Y) < 10 mm

---

### Test 3: 90° Turn Accuracy

**Expected**: Robot rotates 90°; heading changes from 0° to 90°; pen tip traces arc.

```cpp
void testTurnAccuracy() {
    Serial.println("\n[TEST] 90° turn accuracy test...");
    Serial.println("Robot will rotate in place. Watch heading value.");
    
    pose.reset();
    float initialHeading = pose.getHeading();
    
    // Rotate at 45 deg/s (= 0.785 rad/s)
    drive.setLinearVelocity(0.0f);      // mm/s
    drive.setAngularVelocity(0.785f);   // rad/s (45 deg/s)
    
    // 90 degrees = π/2 radians; at 0.785 rad/s takes ~2 seconds
    uint32_t startTime = millis();
    while (millis() - startTime < 2300) {
        pose.update();
        
        if ((millis() - startTime) % 300 == 0) {
            float headingDeg = (pose.getHeading() - initialHeading) * 180 / PI;
            Serial.printf("[TURN] Time=%ldms, Heading change=%.1f°\n",
                          millis() - startTime, headingDeg);
        }
        delay(10);
    }
    
    // Stop
    drive.stop();
    
    float finalHeading = pose.getHeading();
    float headingChange = (finalHeading - initialHeading) * 180 / PI;
    
    // Normalize to -180 to 180
    if (headingChange > 180) headingChange -= 360;
    if (headingChange < -180) headingChange += 360;
    
    Serial.printf("[RESULT] Heading change: %.1f° (target ±90°)\n", headingChange);
    
    if (abs(headingChange - 90.0) < 5.0) {
        Serial.println("[PASS] Turn accuracy excellent!");
    } else if (abs(headingChange - 90.0) < 15.0) {
        Serial.println("[WARN] Turn accuracy acceptable");
    } else {
        Serial.println("[FAIL] Significant turn error - check wheel base calibration");
    }
}
```

**Expected Output**:
```
[TEST] 90° turn accuracy test...
[TURN] Time=300ms, Heading change=13.2°
[TURN] Time=600ms, Heading change=26.5°
[TURN] Time=900ms, Heading change=39.8°
[TURN] Time=1200ms, Heading change=53.1°
[TURN] Time=1500ms, Heading change=66.4°
[TURN] Time=1800ms, Heading change=79.7°
[TURN] Time=2100ms, Heading change=93.0°
[RESULT] Heading change: 93.0° (target ±90°)
[PASS] Turn accuracy excellent!
```

**Pass Criteria**: Heading change within ±5° of target

---

### Test 4: Square Path Trajectory

**Expected**: Robot traces square (500 mm × 500 mm); returns to start.

```cpp
void testSquarePath() {
    Serial.println("\n[TEST] Square path test (500 mm × 500 mm)...");
    
    pose.reset();
    
    struct Point {
        float x, y;
    };
    Vector<Point> path;
    
    // Move forward 500 mm
    Serial.println("[SQUARE] Moving forward...");
    driveDistance(500.0f, 50.0f);  // 500 mm at 50 mm/s
    path.push_back({pose.getPenX(), pose.getPenY()});
    
    delay(1000);
    pose.update();
    
    // Turn left 90°
    Serial.println("[SQUARE] Turning left...");
    turnAngle(PI/2, 0.5f);  // 90° at 0.5 rad/s
    delay(1000);
    
    // Move forward 500 mm
    Serial.println("[SQUARE] Moving forward...");
    driveDistance(500.0f, 50.0f);
    path.push_back({pose.getPenX(), pose.getPenY()});
    
    // ... repeat for 3 more sides ...
    
    // Final position should be near origin
    float finalError = sqrt(pose.getPenX()*pose.getPenX() + 
                            pose.getPenY()*pose.getPenY());
    Serial.printf("[RESULT] Closure error: %.1f mm (target < 20 mm)\n", finalError);
    
    if (finalError < 20.0) {
        Serial.println("[PASS] Square path traced with good closure!");
    } else {
        Serial.println("[WARN] Significant closure error - motor/encoder sync issue?");
    }
}
```

---

### Test 5: Ghost Pose Comparison

**Expected**: Ghost pose (odometry only) drifts more than real pose (IMU-fused).

```cpp
void testGhostPose() {
    Serial.println("\n[TEST] Real vs Ghost pose comparison (30 second forward motion)...");
    
    pose.reset();
    
    // Move forward
    drive.setLinearVelocity(50.0f);
    drive.setAngularVelocity(0.0f);
    
    uint32_t startTime = millis();
    float maxHeadingDrift = 0;
    
    while (millis() - startTime < 30000) {
        pose.update();
        
        if ((millis() - startTime) % 5000 == 0) {
            float realX = pose.getPenX();
            float realY = pose.getPenY();
            float ghostX = pose.getGhostX();
            float ghostY = pose.getGhostY();
            
            float realDistance = sqrt(realX*realX + realY*realY);
            float ghostDistance = sqrt(ghostX*ghostX + ghostY*ghostY);
            float driftError = abs(realDistance - ghostDistance);
            
            float realHeading = pose.getHeading() * 180 / PI;
            float ghostHeading = pose.getGhostHeading() * 180 / PI;
            float headingDrift = abs(realHeading - ghostHeading);
            
            if (headingDrift > maxHeadingDrift) maxHeadingDrift = headingDrift;
            
            Serial.printf("[COMPARE] Time=%ldms\n", millis() - startTime);
            Serial.printf("  Real: (%.1f, %.1f) mm, θ=%.1f°\n", realX, realY, realHeading);
            Serial.printf("  Ghost: (%.1f, %.1f) mm, θ=%.1f°\n", ghostX, ghostY, ghostHeading);
            Serial.printf("  Drift: %.1f mm, Heading drift: %.1f°\n", driftError, headingDrift);
        }
        
        delay(10);
    }
    
    drive.stop();
    
    Serial.printf("[RESULT] Max heading drift (Real vs Ghost): %.1f°\n", maxHeadingDrift);
    Serial.println("[PASS] Ghost pose provides drift visualization!");
}
```

---

## ✅ Verification Checklist: Step 3

- [ ] **Static Position**: No drift detected over 10 seconds at rest
- [ ] **Straight-Line Accuracy**: 200 mm motion has < 5 mm lateral error, < 10 mm distance error
- [ ] **Turn Accuracy**: 90° turn has ± 5° error
- [ ] **Square Path**: Closure error < 20 mm for 500×500 mm square
- [ ] **Ghost Pose**: Ghost pose shows more drift than real pose (IMU-fused)
- [ ] **Encoder Synchronization**: Left and right motors maintain < 3% speed difference

---

## 📊 Expected Data Format (for web visualization)

Telemetry sent to web interface:

```json
{
  "pose": {
    "pen_x": 150.2,
    "pen_y": 285.5,
    "heading_deg": 45.3,
    "axle_x": 10.5,
    "axle_y": 155.2
  },
  "ghost_pose": {
    "x": 152.1,
    "y": 287.2,
    "heading_deg": 45.8
  },
  "motors": {
    "left_speed_mm_s": 50.1,
    "right_speed_mm_s": 49.8
  },
  "imu": {
    "heading_deg": 45.3,
    "pitch_deg": -0.5,
    "roll_deg": 0.2
  }
}
```

---

## 🎯 Next Steps

**Phase 3 Complete!**
→ Move to [girobot_step4.md](girobot_step4.md): **Motion Commands & PID Enhancement**

In Phase 4, you will:
1. Enhance MotionController with full PID (Ki, Kd terms)
2. Implement cross-track error feedback
3. Test waypoint following accuracy
4. Validate heading hold during straight motion

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate (testing & tuning)  
**Hardware Required**: ESP32, motors, encoders, IMU, measuring tape  

Last Updated: May 11, 2026
