# GiRobot Step 4: Motion Commands & PID Enhancement

> **Phase 4**: Enhance MotionController with full PID control, implement cross-track error feedback, and test precise motion commands (go straight 200mm, rotate 90°, follow paths).

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand current MotionController (waypoint following, basic mode switching)
2. ✅ **ENHANCE** MotionController with full PID (Kp, Ki, Kd for cross-track error)
3. ✅ Implement heading hold during straight-line motion
4. ✅ Test precise distance commands: `goStraight(200 mm)`
5. ✅ Test rotation commands: `rotate(90°)`
6. ✅ Validate waypoint following with 3mm tolerance
7. ✅ Tune PID gains for minimal oscillation and fast settling

---

## 🔧 PID Control Theory

### Why PID is Needed

**Problem**: Motor speeds differ slightly due to:
- Friction variations on paper
- Manufacturing tolerance (motor gearing)
- Weight distribution

**Result**: Robot drifts sideways or rotates unexpectedly

**PID Solution**: Measure error (cross-track deviation or heading error) and adjust motor speeds automatically

### PID Formula

```
error = desired_value - measured_value

output = Kp × error                          [Proportional]
       + Ki × ∫error dt                      [Integral]
       + Kd × d(error)/dt                    [Derivative]

steering_command = left_speed - (output × wheelbase_compensation)
```

### Component Roles

| Term | Effect | Purpose |
|------|--------|---------|
| **Kp** | Proportional gain | Fast response to large errors (main controller) |
| **Ki** | Integral gain | Eliminates steady-state error (slow correction) |
| **Kd** | Derivative gain | Damping, reduces overshoot |

### Recommended Starting Values

```
Kp = 0.5       (steering gain per mm lateral error)
Ki = 0.02      (integral windup per mm-second)
Kd = 0.1       (damping per mm/s error rate)

Adjust based on observed behavior:
- Too much Kp → Oscillation (zigzag)
- Too little Kp → Slow response
- Too much Ki → Integral windup, long settling
- Too much Kd → Noise sensitivity
```

---

## 📖 Enhancing MotionController.cpp

Current [src/MotionController.cpp](../src/MotionController.cpp) is **partially implemented** with:
- ✅ Waypoint following (basic)
- ✅ Mode switching (MANUAL, AUTO_PATH, IDLE)
- ✅ Cross-track error calculation
- ❌ Missing full PID integration (Ki, Kd terms)

### Enhancement: Add Full PID Implementation

Add to MotionController.h header:

```cpp
class MotionController {
private:
    struct PIDState {
        float errorPrev = 0;           // Last error for derivative term
        float errorIntegral = 0;       // Accumulated error for integral term
        uint32_t lastUpdateTime = 0;
    };
    
    PIDState crossTrackPID;            // PID for lateral correction
    PIDState headingPID;               // PID for heading control
    
public:
    // PID gains (tunable)
    float Kp_crosstrack = 0.5f;        // Proportional
    float Ki_crosstrack = 0.02f;       // Integral
    float Kd_crosstrack = 0.1f;        // Derivative
    
    float Kp_heading = 0.3f;
    float Ki_heading = 0.01f;
    float Kd_heading = 0.05f;
};
```

Add to MotionController.cpp:

```cpp
float MotionController::computePID(PIDState &pid, float error, 
                                    float Kp, float Ki, float Kd) {
    uint32_t now = millis();
    float dt = (now - pid.lastUpdateTime) / 1000.0f;  // seconds
    pid.lastUpdateTime = now;
    
    // Proportional term
    float pTerm = Kp * error;
    
    // Integral term (accumulate error over time)
    pid.errorIntegral += error * dt;
    
    // Anti-windup: clamp integral
    if (pid.errorIntegral > 10.0f) pid.errorIntegral = 10.0f;
    if (pid.errorIntegral < -10.0f) pid.errorIntegral = -10.0f;
    
    float iTerm = Ki * pid.errorIntegral;
    
    // Derivative term (rate of error change)
    float dError_dt = (error - pid.errorPrev) / dt;
    pid.errorPrev = error;
    
    float dTerm = Kd * dError_dt;
    
    // Sum all terms
    return pTerm + iTerm + dTerm;
}
```

Update the motion control loop:

```cpp
void MotionController::update() {
    switch (mode) {
        case Mode::AUTO_PATH:
            updateWaypointFollowing();
            break;
        case Mode::MANUAL:
            // Manual joystick control (no PID)
            break;
        case Mode::IDLE:
            drive.stop();
            break;
    }
}

void MotionController::updateWaypointFollowing() {
    // Get current pose
    float currentX = pose.getPenX();
    float currentY = pose.getPenY();
    float currentHeading = pose.getHeading();
    
    // Get target waypoint
    float targetX = currentWaypoint.x;
    float targetY = currentWaypoint.y;
    
    // Calculate cross-track error
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float distanceToTarget = sqrt(dx*dx + dy*dy);
    
    // Desired heading to target
    float desiredHeading = atan2(dy, dx);
    float headingError = desiredHeading - currentHeading;
    
    // Normalize heading error to [-π, π]
    if (headingError > PI) headingError -= 2*PI;
    if (headingError < -PI) headingError += 2*PI;
    
    // Calculate cross-track error (perpendicular distance from path)
    float pathHeading = atan2(dy, dx);
    float crossTrackError = dx * sin(pathHeading) - dy * cos(pathHeading);
    
    // Apply PID corrections
    float crossTrackCorrection = computePID(crossTrackPID, 
                                             crossTrackError,
                                             Kp_crosstrack, Ki_crosstrack, Kd_crosstrack);
    
    float headingCorrection = computePID(headingPID,
                                         headingError,
                                         Kp_heading, Ki_heading, Kd_heading);
    
    // Set motor speeds with corrections
    float linearVelocity = 50.0f;  // mm/s (tunable)
    float angularVelocity = crossTrackCorrection + headingCorrection;
    
    // Clamp angular velocity
    if (angularVelocity > maxAngularSpeed) angularVelocity = maxAngularSpeed;
    if (angularVelocity < -maxAngularSpeed) angularVelocity = -maxAngularSpeed;
    
    drive.setLinearVelocity(linearVelocity);
    drive.setAngularVelocity(angularVelocity);
    
    // Check if waypoint reached
    if (distanceToTarget < 3.0f) {  // 3mm tolerance
        advanceToNextWaypoint();
    }
}
```

---

## 🧪 Testing Procedure

### Test 1: Straight-Line Heading Hold

**Expected Behavior**: Robot maintains heading while moving forward (< 5° drift over 1 meter).

```cpp
void testHeadingHold() {
    Serial.println("\n[TEST] Straight-line heading hold (1000 mm)...");
    
    pose.reset();
    motion.setMode(MotionController::Mode::AUTO_PATH);
    
    // Set single waypoint 1000 mm forward
    MotionWaypoint target = {1000.0f, 0.0f};
    motion.setWaypoints(&target, 1);
    
    uint32_t startTime = millis();
    float maxHeadingDeviation = 0;
    
    while (!motion.isPathComplete() && (millis() - startTime < 25000)) {
        pose.update();
        motion.update();
        
        if ((millis() - startTime) % 2000 == 0) {
            float heading = pose.getHeading() * 180 / PI;
            Serial.printf("[HEADING] Time=%ldms, X=%.1f mm, Heading=%.1f°\n",
                          millis() - startTime, pose.getPenX(), heading);
            
            if (abs(heading) > maxHeadingDeviation) {
                maxHeadingDeviation = abs(heading);
            }
        }
        
        delay(10);
    }
    
    motion.stop();
    
    Serial.printf("[RESULT] Max heading deviation: %.1f° (target < 5°)\n", 
                  maxHeadingDeviation);
    
    if (maxHeadingDeviation < 5.0) {
        Serial.println("[PASS] Excellent heading hold!");
    } else {
        Serial.println("[WARN] Heading deviation detected - may need PID tuning");
    }
}
```

---

### Test 2: Precise Distance Command

**Expected**: `goStraight(200 mm)` moves exactly 200 ± 5 mm.

```cpp
void testPreciseDistance() {
    Serial.println("\n[TEST] Precise distance command (200 mm)...");
    
    pose.reset();
    motion.goStraight(200.0f);  // Request 200 mm forward
    
    uint32_t startTime = millis();
    while (!motion.isCommandComplete() && (millis() - startTime < 10000)) {
        pose.update();
        motion.update();
        
        float distance = sqrt(pose.getPenX()*pose.getPenX() + 
                              pose.getPenY()*pose.getPenY());
        
        if ((millis() - startTime) % 500 == 0) {
            Serial.printf("[DIST] Time=%ldms, Distance=%.1f mm\n",
                          millis() - startTime, distance);
        }
        
        delay(10);
    }
    
    float finalDistance = sqrt(pose.getPenX()*pose.getPenX() + 
                               pose.getPenY()*pose.getPenY());
    float error = abs(finalDistance - 200.0f);
    
    Serial.printf("[RESULT] Final distance: %.1f mm, Error: %.1f mm\n",
                  finalDistance, error);
    
    if (error < 5.0) {
        Serial.println("[PASS] Distance accuracy excellent!");
    } else if (error < 15.0) {
        Serial.println("[WARN] Acceptable accuracy");
    } else {
        Serial.println("[FAIL] Significant error - check PID gains");
    }
}
```

---

### Test 3: Precise Rotation Command

**Expected**: `rotate(90°)` rotates exactly 90 ± 2°.

```cpp
void testPreciseRotation() {
    Serial.println("\n[TEST] Precise rotation command (90°)...");
    
    pose.reset();
    motion.rotate(PI/2);  // Request 90° rotation
    
    uint32_t startTime = millis();
    while (!motion.isCommandComplete() && (millis() - startTime < 10000)) {
        pose.update();
        motion.update();
        
        float headingDeg = pose.getHeading() * 180 / PI;
        
        if ((millis() - startTime) % 500 == 0) {
            Serial.printf("[ROT] Time=%ldms, Heading=%.1f°\n",
                          millis() - startTime, headingDeg);
        }
        
        delay(10);
    }
    
    float finalHeading = pose.getHeading() * 180 / PI;
    float error = abs(finalHeading - 90.0f);
    
    Serial.printf("[RESULT] Final heading: %.1f°, Error: %.1f°\n",
                  finalHeading, error);
    
    if (error < 2.0) {
        Serial.println("[PASS] Rotation accuracy excellent!");
    } else if (error < 5.0) {
        Serial.println("[WARN] Acceptable accuracy");
    } else {
        Serial.println("[FAIL] Significant error - check PID gains");
    }
}
```

---

### Test 4: Waypoint Following (Square)

**Expected**: Robot follows 4 waypoints (square path) with < 10 mm path deviation.

```cpp
void testWaypointFollowing() {
    Serial.println("\n[TEST] Waypoint following (500 mm × 500 mm square)...");
    
    pose.reset();
    
    // Define square waypoints
    MotionWaypoint waypoints[] = {
        {500.0f, 0.0f},      // Forward
        {500.0f, 500.0f},    // Right
        {0.0f, 500.0f},      // Back
        {0.0f, 0.0f}         // Home
    };
    
    motion.setMode(MotionController::Mode::AUTO_PATH);
    motion.setWaypoints(waypoints, 4);
    
    uint32_t startTime = millis();
    float maxPathDeviation = 0;
    
    while (!motion.isPathComplete() && (millis() - startTime < 60000)) {
        pose.update();
        motion.update();
        
        // Calculate current path error
        float currentX = pose.getPenX();
        float currentY = pose.getPenY();
        float pathError = motion.getCrossTrackError();
        
        if (pathError > maxPathDeviation) {
            maxPathDeviation = pathError;
        }
        
        if ((millis() - startTime) % 5000 == 0) {
            Serial.printf("[PATH] Time=%ldms, X=%.1f mm, Y=%.1f mm, Error=%.1f mm\n",
                          millis() - startTime, currentX, currentY, pathError);
        }
        
        delay(10);
    }
    
    float closureError = sqrt(pose.getPenX()*pose.getPenX() + 
                              pose.getPenY()*pose.getPenY());
    
    Serial.printf("[RESULT] Max path deviation: %.1f mm, Closure error: %.1f mm\n",
                  maxPathDeviation, closureError);
    
    if (maxPathDeviation < 10.0 && closureError < 15.0) {
        Serial.println("[PASS] Waypoint following excellent!");
    } else {
        Serial.println("[WARN] Path deviation detected - review PID tuning");
    }
}
```

---

### Test 5: PID Tuning Guide

**Observation-Based Tuning**:

| Symptom | Adjustment |
|---------|------------|
| Robot overshoots, oscillates | ↓ Kp or ↑ Kd |
| Robot too slow to respond | ↑ Kp |
| Steady-state error remains | ↑ Ki |
| Response jerky/noisy | ↓ Kd |

**Recommended Tuning Sequence**:
1. Start with Kp only (Ki=0, Kd=0)
2. Increase Kp until slight oscillation appears
3. Reduce Kp slightly (back off by 20%)
4. Add Kd to damp oscillation
5. Add small Ki to eliminate steady-state error

**Example Tuning Session**:

```
Initial: Kp=0.5, Ki=0.02, Kd=0.1
Result: Robot oscillates side-to-side

Adjust: Kp=0.3, Ki=0.02, Kd=0.1
Result: Better, but slow response

Adjust: Kp=0.35, Ki=0.02, Kd=0.15
Result: Smooth response, no oscillation
✅ FINAL TUNING
```

---

## ✅ Verification Checklist: Step 4

- [ ] **Heading Hold**: < 5° deviation over 1 meter forward motion
- [ ] **Precise Distance**: 200 mm command has < 5 mm error
- [ ] **Precise Rotation**: 90° command has < 2° error
- [ ] **Waypoint Following**: Square path has < 10 mm path deviation
- [ ] **Path Closure**: Square returns to origin with < 15 mm error
- [ ] **No Oscillation**: Robot tracks smoothly without zigzag
- [ ] **PID Gains Tuned**: Kp, Ki, Kd values stored in Config.h

---

## 📊 Sample Serial Output (Successful Phase 4)

```
[TEST] Straight-line heading hold (1000 mm)...
[HEADING] Time=2000ms, X=100.2 mm, Heading=0.3°
[HEADING] Time=4000ms, X=199.8 mm, Heading=-0.1°
[HEADING] Time=6000ms, X=300.1 mm, Heading=0.2°
[HEADING] Time=8000ms, X=400.0 mm, Heading=-0.2°
[RESULT] Max heading deviation: 0.3° (target < 5°)
[PASS] Excellent heading hold!

[TEST] Precise distance command (200 mm)...
[DIST] Time=500ms, Distance=50.2 mm
[DIST] Time=1000ms, Distance=100.1 mm
[DIST] Time=1500ms, Distance=150.0 mm
[DIST] Time=2000ms, Distance=200.3 mm
[RESULT] Final distance: 200.3 mm, Error: 0.3 mm
[PASS] Distance accuracy excellent!

[TEST] Waypoint following (500 mm × 500 mm square)...
[PATH] Time=5000ms, X=250.1 mm, Y=1.2 mm, Error=1.2 mm
[PATH] Time=10000ms, X=500.0 mm, Y=0.5 mm, Error=0.5 mm
[PATH] Time=15000ms, X=500.2 mm, Y=250.3 mm, Error=2.1 mm
[PATH] Time=20000ms, X=500.1 mm, Y=500.0 mm, Error=0.8 mm
[RESULT] Max path deviation: 2.1 mm, Closure error: 2.3 mm
[PASS] Waypoint following excellent!

[OK] All motion tests passed! PID tuned and ready for drawing operations.
```

---

## 🎯 Next Steps

**Phase 4 Complete!**
→ Move to [girobot_step5.md](girobot_step5.md): **Pen Features & Drawing Logic**

In Phase 5, you will:
1. Implement pen contact detection
2. Build pen trace buffering
3. Create drawing command handlers
4. Test line drawing accuracy

---

**Estimated Time**: 3 hours (including PID tuning)  
**Difficulty**: ⭐⭐⭐ Advanced (control theory & tuning)  
**Hardware Required**: ESP32, motors, encoders, IMU, measuring tape  

Last Updated: May 11, 2026
