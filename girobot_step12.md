# GiRobot Step 12: Integration Testing & Tuning

> **Phase 12**: Full system integration test, PID tuning refinement, performance profiling, and final calibration workflow.

---

## 📋 Objectives

1. ✅ Verify all modules work together
2. ✅ Perform full-system tests (motors → IMU → path → web)
3. ✅ Refine PID gains for optimal performance
4. ✅ Profile system performance (CPU load, timing)
5. ✅ Document calibration workflow

---

## 🧪 Full System Integration Tests

### Test 1: End-to-End Motion

**Expected**: Robot completes square path with < 20mm closure error.

```cpp
void testFullIntegration() {
    Serial.println("\n[TEST] Full system integration (square path)...");
    Serial.println("Follow these steps:");
    Serial.println("1. Place robot on paper with pen touching");
    Serial.println("2. Mark starting position");
    Serial.println("3. Robot will draw 500x500mm square");
    Serial.println("4. Mark ending position and measure closure error");
    
    pose.reset();
    motion.drawSquare(0, 0, 500);
    
    uint32_t startTime = millis();
    while (!motion.isPathComplete() && (millis() - startTime < 120000)) {
        pose.update();
        motion.update();
        
        if ((millis() - startTime) % 10000 == 0) {
            float progress = motion.getPathProgress() * 100;
            Serial.printf("[PROGRESS] %.1f%% complete\n", progress);
        }
        
        delay(10);
    }
    
    float finalX = pose.getPenX();
    float finalY = pose.getPenY();
    float closureError = sqrt(finalX*finalX + finalY*finalY);
    
    Serial.printf("[RESULT] Closure: (%.1f, %.1f) mm = %.1f mm error\n",
                  finalX, finalY, closureError);
    
    if (closureError < 20.0) {
        Serial.println("[PASS] Full integration excellent!");
    } else {
        Serial.println("[WARN] Consider PID tuning (see step 4)");
    }
}
```

### Test 2: Drawing Accuracy

**Expected**: Drawn circle has radius within ±5mm of target.

```cpp
void testDrawingAccuracy() {
    Serial.println("\n[TEST] Drawing accuracy (50mm circle)...");
    
    pose.reset();
    motion.drawCircle(0, 0, 50.0f);
    
    float minRadius = 999, maxRadius = 0;
    
    while (!motion.isPathComplete()) {
        pose.update();
        motion.update();
        
        float radius = sqrt(pose.getPenX()*pose.getPenX() + 
                            pose.getPenY()*pose.getPenY());
        minRadius = min(minRadius, radius);
        maxRadius = max(maxRadius, radius);
        
        delay(10);
    }
    
    float accuracy = (maxRadius - minRadius) / 2.0f;
    Serial.printf("[ACCURACY] Radius: %.1f ± %.1f mm (target 50 ± 3 mm)\n",
                  (maxRadius + minRadius) / 2, accuracy);
    
    if (accuracy < 3.0) {
        Serial.println("[PASS] Drawing accuracy excellent!");
    }
}
```

### Test 3: Web Interface Responsiveness

**Expected**: Web commands execute within 200ms of button click.

```javascript
// In data/script.js
async function testWebResponseTime() {
    console.log('[TEST] Web interface response time...');
    
    const commands = [
        {cmd: 'STOP'},
        {cmd: 'DRIVE', linear_mm_s: 50, angular_rad_s: 0},
        {cmd: 'STOP'},
        {cmd: 'DRAW_CIRCLE', x: 0, y: 0, radius: 50}
    ];
    
    for (const cmd of commands) {
        const t0 = performance.now();
        robot.send(cmd);
        
        // Wait for telemetry indicating command execution
        await new Promise(resolve => setTimeout(resolve, 200));
        
        const t1 = performance.now();
        console.log(`[TIME] ${cmd.cmd}: ${(t1-t0).toFixed(1)}ms`);
    }
}
```

---

## 🎛️ PID Tuning Refinement

### Observation-Based Tuning

Run straight-line test and observe behavior:

```
Robot behavior → Adjustment
────────────────────────────
Overshoots zigzag → Reduce Kp, increase Kd
Too slow response → Increase Kp
Steady-state error → Increase Ki
Response jerky → Reduce Kd
```

### Systematic Tuning Procedure

1. **Set baseline**: Kp=0.3, Ki=0.02, Kd=0.1
2. **Test straight 500mm**: Note deviation
3. **Adjust Kp**:
   - If zigzag: reduce to 0.2
   - If too slow: increase to 0.4
4. **Adjust Kd** (after Kp stable):
   - If oscillating: increase to 0.15
   - If overshooting: increase to 0.2
5. **Adjust Ki** (last):
   - If steady-state error remains: increase to 0.03

**Tested range**: Kp: 0.2-0.5, Ki: 0.01-0.05, Kd: 0.05-0.2

### Store Final Values in Config.h

```cpp
// In include/Config.h (add these):
constexpr float MotionController::DEFAULT_KP = 0.35f;
constexpr float MotionController::DEFAULT_KI = 0.02f;
constexpr float MotionController::DEFAULT_KD = 0.12f;
```

---

## ⏱️ Performance Profiling

### CPU Load Testing

```cpp
void profileSystemPerformance() {
    Serial.println("\n[PROFILE] System performance monitoring...");
    
    uint32_t loopStartTime = millis();
    uint32_t maxLoopTime = 0;
    uint32_t loopCount = 0;
    
    for (int i = 0; i < 10000; i++) {  // Run 10k loop iterations
        uint32_t iterStart = micros();
        
        pose.update();
        motion.update();
        nav.update();
        
        uint32_t iterTime = micros() - iterStart;
        maxLoopTime = max(maxLoopTime, iterTime);
        loopCount++;
    }
    
    uint32_t totalTime = millis() - loopStartTime;
    float avgLoopTime = (float)totalTime / loopCount;
    
    Serial.printf("[TIMING] Loop time: avg=%.2fms, max=%.2fms\n",
                  avgLoopTime, maxLoopTime / 1000.0f);
    
    Serial.printf("[TIMING] Loop frequency: %.1f Hz\n",
                  1000.0f / avgLoopTime);
    
    if (avgLoopTime < 15.0f) {
        Serial.println("[PASS] System timing excellent (>60 Hz)");
    } else if (avgLoopTime < 25.0f) {
        Serial.println("[WARN] System timing acceptable (>40 Hz)");
    } else {
        Serial.println("[FAIL] System timing marginal (<40 Hz)");
    }
}
```

### Memory Usage

```cpp
void profileMemoryUsage() {
    Serial.println("\n[PROFILE] Memory usage...");
    
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    float usedPercent = 100.0f * (1.0f - (float)freeHeap / totalHeap);
    
    Serial.printf("[MEMORY] Free: %u bytes / %u bytes (%.1f%% used)\n",
                  freeHeap, totalHeap, usedPercent);
    
    if (usedPercent < 50.0f) {
        Serial.println("[PASS] Memory usage healthy");
    } else if (usedPercent < 80.0f) {
        Serial.println("[WARN] Memory usage moderate");
    } else {
        Serial.println("[FAIL] Memory usage critical");
    }
}
```

---

## 📋 Complete Calibration Workflow

Perform in this order:

1. **Physical Setup**
   - [ ] Robot on flat, level surface
   - [ ] Clear of metal objects
   - [ ] Paper ready for drawing test

2. **Firmware Upload**
   - [ ] Build: `platformio run -e esp32_real`
   - [ ] Upload: `platformio run -e esp32_real --target upload`

3. **IMU Calibration** (from serial menu)
   - [ ] Gyro calibration (keep still 5s)
   - [ ] Mag calibration (rotate 360° slowly 15s)

4. **Motor Tuning**
   - [ ] Test motor speed ramps
   - [ ] Verify encoder feedback
   - [ ] Confirm motor synchronization

5. **PID Tuning**
   - [ ] Run straight-line test
   - [ ] Adjust Kp, Ki, Kd per observation
   - [ ] Repeat until satisfied

6. **Battery Check**
   - [ ] Verify voltage reading
   - [ ] Check low-battery threshold

7. **Web Interface**
   - [ ] Connect to WiFi "RobotWifi"
   - [ ] Access dashboard at 192.168.4.1
   - [ ] Test motion commands

8. **Drawing Test**
   - [ ] Draw circle 50mm radius
   - [ ] Draw square 500x500mm
   - [ ] Write "TEST"

9. **Performance Check**
   - [ ] Monitor loop timing (should be > 60 Hz)
   - [ ] Check memory usage (< 80%)
   - [ ] Verify no crashes

10. **Documentation**
    - [ ] Record final PID values
    - [ ] Note any calibration offsets
    - [ ] Document motor behavior

---

## ✅ Final Verification Checklist: Step 12

- [ ] **Square Path**: Closure error < 20mm
- [ ] **Circle Drawing**: Radius accuracy ±3mm
- [ ] **Text Output**: Legible characters, proper spacing
- [ ] **Web Commands**: All buttons execute < 200ms response
- [ ] **PID Tuned**: Motion smooth, no oscillation
- [ ] **System Timing**: Loop > 60 Hz average
- [ ] **Memory Usage**: < 80% heap utilization
- [ ] **Battery Reading**: Accurate within 0.2V
- [ ] **WiFi Stable**: No disconnections during test
- [ ] **No Crashes**: 1-hour stress test completes successfully

---

## 🎓 Stress Test: 1-Hour Autonomous Run

```cpp
void runStressTest() {
    Serial.println("\n[STRESS] 1-hour autonomous test starting...");
    Serial.println("Robot will repeat drawing operations for 1 hour.");
    
    uint32_t startTime = millis();
    int iterations = 0;
    
    while (millis() - startTime < 3600000) {  // 1 hour
        // Draw circle
        motion.drawCircle(0, 0, 50);
        while (!motion.isPathComplete()) {
            pose.update();
            motion.update();
            delay(10);
        }
        
        // Draw triangle
        motion.drawTriangle(0, 80, -70, -40, 70, -40);
        while (!motion.isPathComplete()) {
            pose.update();
            motion.update();
            delay(10);
        }
        
        // Move around
        motion.goStraight(200);
        while (!motion.isPathComplete()) {
            pose.update();
            motion.update();
            delay(10);
        }
        
        iterations++;
        
        if (iterations % 10 == 0) {
            uint32_t elapsedMin = (millis() - startTime) / 60000;
            Serial.printf("[STRESS] %d cycles complete, %ld minutes elapsed\n",
                          iterations, elapsedMin);
            profileMemoryUsage();
        }
    }
    
    Serial.printf("[COMPLETE] Stress test finished after %d cycles\n", iterations);
    Serial.println("[SUCCESS] Robot passed 1-hour stress test!");
}
```

---

## 📊 Final Performance Report Template

```
╔════════════════════════════════════════════════════════════════╗
║           GiRobot System Performance Report                    ║
╠════════════════════════════════════════════════════════════════╣
║ Firmware Version: 0.1.1                                        ║
║ Test Date: [Date]                                              ║
║ Test Duration: [Duration]                                      ║
├────────────────────────────────────────────────────────────────┤
║ Motion Performance                                              ║
║ - Straight-line accuracy: ±[X] mm deviation                    ║
║ - Circle radius accuracy: [R] ± [E] mm                         ║
║ - Square closure error: [E] mm                                 ║
║ - PID Gains: Kp=[Kp], Ki=[Ki], Kd=[Kd]                        ║
├────────────────────────────────────────────────────────────────┤
║ System Performance                                              ║
║ - Loop timing: [T] ms average                                  ║
║ - Loop frequency: [F] Hz                                       ║
║ - Memory usage: [M]% of heap                                   ║
│ - Uptime: [U] hours without crash                             ║
├────────────────────────────────────────────────────────────────┤
║ Sensor Calibration                                              ║
║ - Gyro offset: ([X], [Y], [Z]) deg/s                           ║
║ - Mag offset: ([X], [Y], [Z]) µT                               ║
║ - Heading drift: [D] °/min (stationary)                        ║
├────────────────────────────────────────────────────────────────┤
║ Web Interface                                                   ║
║ - WiFi connection: ✓ Stable                                    ║
║ - Dashboard: ✓ Responsive                                      ║
║ - Telemetry latency: [L] ms                                    ║
├────────────────────────────────────────────────────────────────┤
║ Overall Status: ✓ READY FOR DEPLOYMENT                         ║
╚════════════════════════════════════════════════════════════════╝
```

---

## 🎯 Next: Deployment & Operation

**Congratulations! GiRobot is now fully configured and tested.**

You can now:
- ✅ Draw geometric shapes with precision
- ✅ Write text with proper character rendering
- ✅ Control via web interface in real-time
- ✅ Monitor sensor data and position
- ✅ Perform autonomous drawing sequences

---

**Estimated Time**: 3 hours (including stress test)  
**Difficulty**: ⭐⭐⭐ Advanced (system tuning)  
**Milestone**: ✅ PROJECT COMPLETE

Last Updated: May 11, 2026
