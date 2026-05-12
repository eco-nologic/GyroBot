# GiRobot Step 1: Motor & Encoder Control

> **Phase 1**: Verify DC motor operation, test encoder feedback, validate speed ramps, and confirm motor fault detection.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand how DCMotor.cpp controls hardware pins
2. ✅ Test motor speed ramps (acceleration/deceleration)
3. ✅ Verify encoder feedback counts match commanded steps
4. ✅ Validate motor fault detection (speed mismatch warnings)
5. ✅ Confirm PWM deadband (motors start at PWM > 150)
6. ✅ Test both forward and backward directions
7. ✅ Monitor serial telemetry for motor status

---

## 🔌 Hardware Wiring Reference

From [include/Config.h](../include/Config.h):

### Left Motor (DC)
```
GPIO 17 → Motor driver IN1 (direction)
GPIO 16 → Motor driver IN2 (direction)
GPIO 4  → Motor driver EN (PWM speed)
```

### Right Motor (DC)
```
GPIO 19 → Motor driver IN1 (direction)
GPIO 18 → Motor driver IN2 (direction)
GPIO 23 → Motor driver EN (PWM speed)
```

### Left Encoder (Quadrature)
```
GPIO 32 → Encoder channel A
GPIO 33 → Encoder channel B
```

### Right Encoder (Quadrature)
```
GPIO 27 → Encoder channel A
GPIO 14 → Encoder channel B
```

**Motor Driver Typical Setup** (DRV8837 or similar):
- IN1=HIGH, IN2=LOW → Forward
- IN1=LOW, IN2=HIGH → Backward
- IN1=LOW, IN2=LOW → Coast/Brake
- EN=0 → Motor off; EN=255 → Max speed

**Encoder Logic**:
- Read both A and B channels every PWM cycle
- Determine direction from A→B phase relationship
- Count pulses: 1070 pulses = 1 full wheel revolution

---

## 📖 Understanding DCMotor.cpp

Review [src/DCMotor.cpp](../src/DCMotor.cpp) to understand:

### 1. **Class Declaration** (DCMotor.h)
```cpp
class DCMotor : public IMotor {
public:
    DCMotor(int id, int enPin, int in1Pin, int in2Pin, 
            int pwm1Pin, int pwm2Pin, int encA, int encB);
    
    // IMotor interface implementation:
    void begin();
    void setSpeed(float stepsPerSecond);
    void setMaxSpeed(float stepsPerSecond);
    void setAcceleration(float stepsPerSecSq);
    void moveTo(long absoluteSteps);
    void moveRelative(long relativeSteps);
    long getCurrentPosition();
    void setCurrentPosition(long position);
    bool isRunning();
    void run();
    void stop();
    float getCurrentSpeed();
    
private:
    int motorId;
    int enablePin, in1Pin, in2Pin;
    int pwm1Pin, pwm2Pin;
    int encAPinNum, encBPinNum;
    
    // Encoder feedback tracking:
    volatile long encoderCount;      // Updated by ISR on every edge
    long lastEncoderCount;
    
    // Motion profile:
    float targetSpeed;               // steps/second
    float currentSpeed;              // steps/second
    float acceleration;              // steps/second²
    long targetPosition;             // absolute steps
    
    // Fault detection:
    uint32_t lastSpeedCheckTime;
    int speedMismatchCount;
    bool motorFault;
};
```

### 2. **Key Methods**

#### begin()
Initialize GPIO pins and attach encoder ISR:
```cpp
void DCMotor::begin() {
    pinMode(enablePin, OUTPUT);
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);
    
    digitalWrite(enablePin, LOW);   // Start motor disabled
    
    // Encoder ISR: triggered on every A-channel edge
    attachInterrupt(digitalPinToInterrupt(encAPinNum),
                    [this]() { encoderISR(); },
                    CHANGE);
}
```

#### setSpeed(float stepsPerSecond)
Set target speed with acceleration ramping:
```cpp
void DCMotor::setSpeed(float stepsPerSecond) {
    targetSpeed = stepsPerSecond;
    // Actual speed ramps gradually based on acceleration
}
```

#### run()
Called every loop iteration; implements acceleration profile:
```cpp
void DCMotor::run() {
    // 1. Calculate elapsed time since last call
    uint32_t now = millis();
    float dt = (now - lastRunTime) / 1000.0f;  // seconds
    
    // 2. Ramp currentSpeed toward targetSpeed
    if (currentSpeed < targetSpeed) {
        currentSpeed += acceleration * dt;
        if (currentSpeed > targetSpeed) 
            currentSpeed = targetSpeed;
    } else if (currentSpeed > targetSpeed) {
        currentSpeed -= acceleration * dt;
        if (currentSpeed < targetSpeed) 
            currentSpeed = targetSpeed;
    }
    
    // 3. Update motor output (PWM + direction)
    updateMotorOutput(currentSpeed);
    
    // 4. Check encoder feedback for faults
    checkEncoderFault();
}
```

#### updateMotorOutput()
Converts speed to PWM with deadband correction:
```cpp
void DCMotor::updateMotorOutput(float speed) {
    // PWM value: 0-255 (0=off, 255=max)
    int pwmValue = abs((int)speed);
    
    // DEADBAND: Motors don't start until PWM > 150
    if (pwmValue < 150) pwmValue = 0;
    if (pwmValue > 255) pwmValue = 255;
    
    // Direction control:
    if (speed > 0) {
        digitalWrite(in1Pin, HIGH);
        digitalWrite(in2Pin, LOW);   // Forward
    } else if (speed < 0) {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, HIGH);  // Backward
    } else {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, LOW);   // Coast
    }
    
    // Apply PWM speed
    analogWrite(enablePin, pwmValue);
}
```

#### checkEncoderFault()
Verify encoder steps match commanded steps (300ms check interval):
```cpp
void DCMotor::checkEncoderFault() {
    if (millis() - lastSpeedCheckTime > 300) {
        lastSpeedCheckTime = millis();
        
        long expectedSteps = (targetSpeed / stepsPerSecond) * 0.3;  // Expected in 300ms
        long actualSteps = encoderCount - lastEncoderCount;
        
        if (abs(expectedSteps - actualSteps) > threshold) {
            speedMismatchCount++;
            if (speedMismatchCount > 3) {
                motorFault = true;
                // Log error: encoder slippage detected
            }
        } else {
            speedMismatchCount = 0;  // Reset counter if in tolerance
        }
        
        lastEncoderCount = encoderCount;
    }
}
```

---

## 🧪 Testing Procedure

### Test 1: Motor Initialization

**Expected Behavior**: Serial output confirms motor pins initialized

**Steps**:
1. Upload firmware with `platformio run -e esp32_real --target upload`
2. Open serial monitor (115200 baud)
3. Look for boot output confirming motor initialization

**Expected Serial Output**:
```
[I] Firmware Version: 0.1.1
[I] Initializing motors...
[I] Left motor initialized (EN=GPIO4, IN1=GPIO17, IN2=GPIO16)
[I] Right motor initialized (EN=GPIO23, IN1=GPIO19, IN2=GPIO18)
[I] Encoder pins configured (A=GPIO32, B=GPIO33 for left; A=GPIO27, B=GPIO14 for right)
[I] System ready!
```

**If output doesn't appear**: Check USB connection and baud rate.

---

### Test 2: Motor Speed Ramp (Acceleration Profile)

**Expected Behavior**: Motor accelerates smoothly (not instantaneously) to target speed.

**Procedure**:

Add a test function to [src/main.cpp](../src/main.cpp) or create a dedicated test:

```cpp
// In MotorTestSimple.cpp or inline in main()
void testMotorAcceleration() {
    Serial.println("[TEST] Starting motor acceleration test...");
    
    // Set acceleration: 500 steps/second²
    leftMotor.setAcceleration(500.0f);
    
    // Set target speed: 200 steps/second (~19 mm/s)
    leftMotor.setSpeed(200.0f);
    
    // Run motor for 5 seconds, log speed every 0.5 seconds
    uint32_t startTime = millis();
    while (millis() - startTime < 5000) {
        leftMotor.run();
        
        if (millis() % 500 == 0) {  // Every 500ms
            Serial.printf("[ACCEL] Time=%ldms, Speed=%.1f steps/s, Position=%ld steps\n",
                          millis() - startTime,
                          leftMotor.getCurrentSpeed(),
                          leftMotor.getCurrentPosition());
        }
        delay(1);
    }
    
    // Decelerate
    leftMotor.setSpeed(0.0f);
    
    // Let motor coast down
    while (leftMotor.isRunning()) {
        leftMotor.run();
        Serial.printf("[DECEL] Speed=%.1f steps/s\n", leftMotor.getCurrentSpeed());
        delay(50);
    }
    
    Serial.println("[TEST] Motor acceleration test complete.");
}
```

**Expected Behavior**:
- Speed ramps from 0 to 200 steps/s over ~0.4 seconds (200/500 = 0.4s)
- Smooth curve, not stepped jumps
- Motor reaches target speed and maintains it
- On deceleration, speed ramps back to 0 smoothly

**Troubleshooting**:
- If motor doesn't start: Check PWM deadband (threshold ~150)
- If motor jerks: Acceleration too high; reduce to 200.0f steps/s²
- If motor won't stop: Check coast logic (both direction pins LOW)

---

### Test 3: Encoder Feedback Validation

**Expected Behavior**: Encoder counts match commanded steps within 2% tolerance.

**Setup**:

Mark the wheel at a known position. Command motor to move 100 steps (~9.4 mm for 90mm wheel). Measure actual distance with ruler.

```cpp
void testEncoderAccuracy() {
    Serial.println("[TEST] Starting encoder accuracy test...");
    
    // Reset position counter
    leftMotor.setCurrentPosition(0);
    
    // Accelerate to speed
    leftMotor.setAcceleration(300.0f);
    leftMotor.setSpeed(150.0f);  // Slow speed for accuracy
    
    // Move 1000 steps (~94 mm)
    leftMotor.moveTo(1000);
    
    uint32_t startTime = millis();
    while (leftMotor.isRunning() && (millis() - startTime < 15000)) {
        leftMotor.run();
        delay(1);
    }
    
    long finalPosition = leftMotor.getCurrentPosition();
    Serial.printf("[ENCODER] Commanded: 1000 steps, Actual: %ld steps, Error: %.1f%%\n",
                  finalPosition,
                  100.0 * abs(1000 - finalPosition) / 1000.0);
    
    // Stop motor
    leftMotor.setSpeed(0.0f);
    while (leftMotor.isRunning()) {
        leftMotor.run();
        delay(1);
    }
}
```

**Expected Result**:
```
[ENCODER] Commanded: 1000 steps, Actual: 1002 steps, Error: 0.2%
```

**Pass Criteria**: Error < 2%

**If encoder error > 2%**:
- Check encoder wiring (GPIO 32, 33 for left)
- Verify pull-up resistors on encoder pins (4.7k recommended)
- Check ISR frequency (should be ~2140 Hz for 1070 steps/rev at 2 m/s)

---

### Test 4: Motor Fault Detection

**Expected Behavior**: Serial output warns if encoder slippage detected (wheel slip on paper).

**Simulate Fault**:
1. Run motor at speed (200 steps/s)
2. Physically block the wheel (don't let it rotate)
3. Motor should continue applying PWM, but encoder stays static

**Expected Serial Output**:
```
[WARN] Left motor: Speed mismatch detected (expected 60 steps, got 2)
[ERROR] Left motor: Encoder fault! Too many speed mismatches (5/5)
[ERROR] Motion halted: Motor fault detected
```

**Recovery**:
- Motor automatically stops when fault detected
- Check for obstacles (paper jam, motor stall)
- Reset fault flag and retry

---

### Test 5: Forward & Backward Motion

**Expected Behavior**: Motor rotates in correct direction, encoder tracks correctly.

```cpp
void testMotorDirection() {
    Serial.println("[TEST] Testing motor direction...");
    
    leftMotor.setCurrentPosition(0);
    leftMotor.setAcceleration(400.0f);
    
    // Test forward
    Serial.println("[FORWARD] Moving forward...");
    leftMotor.setSpeed(200.0f);
    delay(2000);
    long forwardPos = leftMotor.getCurrentPosition();
    
    // Stop
    leftMotor.setSpeed(0.0f);
    delay(1000);
    
    // Test backward
    Serial.println("[BACKWARD] Moving backward...");
    leftMotor.setSpeed(-200.0f);
    delay(2000);
    long backwardPos = leftMotor.getCurrentPosition();
    
    Serial.printf("[RESULT] Forward: %ld steps, Backward: %ld steps\n",
                  forwardPos, backwardPos);
    
    if (forwardPos > 0 && backwardPos < forwardPos) {
        Serial.println("[PASS] Motor direction control working correctly");
    } else {
        Serial.println("[FAIL] Motor direction control issue - check IN1/IN2 wiring");
    }
}
```

---

### Test 6: Both Motors Synchronized

**Expected Behavior**: Left and right motors move same distance for straight-line driving.

```cpp
void testMotorSynchronization() {
    Serial.println("[TEST] Testing motor synchronization...");
    
    leftMotor.setCurrentPosition(0);
    rightMotor.setCurrentPosition(0);
    
    leftMotor.setAcceleration(300.0f);
    rightMotor.setAcceleration(300.0f);
    
    // Both motors same speed
    leftMotor.setSpeed(150.0f);
    rightMotor.setSpeed(150.0f);
    
    uint32_t startTime = millis();
    while (millis() - startTime < 3000) {
        leftMotor.run();
        rightMotor.run();
        delay(1);
    }
    
    long leftPos = leftMotor.getCurrentPosition();
    long rightPos = rightMotor.getCurrentPosition();
    
    float positionDifference = abs((float)(leftPos - rightPos)) / leftPos * 100.0;
    
    Serial.printf("[SYNC] Left: %ld, Right: %ld, Difference: %.1f%%\n",
                  leftPos, rightPos, positionDifference);
    
    if (positionDifference < 3.0) {
        Serial.println("[PASS] Motors synchronized within 3% tolerance");
    } else {
        Serial.println("[WARN] Motor speeds differ > 3% - may cause turning during straight motion");
    }
}
```

---

## ✅ Verification Checklist: Step 1

- [ ] **Boot Output**: Serial shows motor pins initialized (GPIO 4, 23 for EN pins)
- [ ] **Acceleration Ramp**: Motor speed increases smoothly over ~0.4s to target (no jerking)
- [ ] **Encoder Accuracy**: Encoder step count within 2% of commanded steps
- [ ] **Fault Detection**: Serial warns when wheel is mechanically blocked
- [ ] **Direction Control**: Forward/backward motion verified with correct rotation
- [ ] **Motor Synchronization**: Left and right motors achieve same speed within 3% tolerance
- [ ] **PWM Deadband**: Motor starts moving when PWM > 150 (adjust if needed)

---

## 🐛 Troubleshooting

### Motor Doesn't Move
**Cause**: PWM signal not reaching driver, or wrong pins

**Checks**:
- [ ] Verify GPIO 4 (left EN), GPIO 23 (right EN) are correct in Config.h
- [ ] Use multimeter to check PWM voltage on driver EN pins
- [ ] Confirm motor power supply is connected
- [ ] Check motor driver VCC/GND connections

### Motor Moves Backward Instead of Forward
**Cause**: IN1/IN2 pins reversed

**Fix**: In Config.h, swap:
```cpp
// Before:
constexpr int PinMotorLeftIn1 = 17;
constexpr int PinMotorLeftIn2 = 16;

// After:
constexpr int PinMotorLeftIn1 = 16;
constexpr int PinMotorLeftIn2 = 17;
```

Then rebuild and upload.

### Encoder Counts Don't Match Steps
**Cause**: Encoder wiring reversed or ISR not triggering

**Checks**:
- [ ] Verify encoder wires on GPIO 32/33 (left) and 27/14 (right)
- [ ] Check encoder pull-up resistors (4.7k ohm typical)
- [ ] Try swapping A/B channels in Config.h
- [ ] Serial print ISR call count to verify interrupt firing

### Motor Fault Detected on Startup
**Cause**: Wheel physically jammed or motor fault flag stuck

**Fix**:
```cpp
// In setup(), add:
// leftMotor.clearFault();  // If clearFault() method exists
// Or restart ESP32
```

---

## 📊 Sample Serial Output (Successful Phase 1)

```
[I] Firmware Version: 0.1.1
[I] System Mode: REAL
[I] Motor initialization...
[I] Left motor initialized
[I] Right motor initialized
[I] System ready!

--- Starting Motor Test Suite ---

[TEST] Starting motor acceleration test...
[ACCEL] Time=0ms, Speed=0.0 steps/s, Position=0 steps
[ACCEL] Time=500ms, Speed=200.0 steps/s, Position=100 steps
[ACCEL] Time=1000ms, Speed=200.0 steps/s, Position=300 steps
[ACCEL] Time=1500ms, Speed=200.0 steps/s, Position=500 steps
[ACCEL] Time=2000ms, Speed=200.0 steps/s, Position=700 steps
[ACCEL] Time=2500ms, Speed=200.0 steps/s, Position=900 steps
[ACCEL] Time=3000ms, Speed=200.0 steps/s, Position=1100 steps
[ACCEL] Time=3500ms, Speed=200.0 steps/s, Position=1300 steps
[ACCEL] Time=4000ms, Speed=200.0 steps/s, Position=1500 steps
[ACCEL] Time=4500ms, Speed=200.0 steps/s, Position=1700 steps
[TEST] Motor acceleration test complete.

[TEST] Starting encoder accuracy test...
[ENCODER] Commanded: 1000 steps, Actual: 1003 steps, Error: 0.3%
[PASS] Encoder accuracy within tolerance

[TEST] Testing motor direction...
[FORWARD] Moving forward...
[BACKWARD] Moving backward...
[RESULT] Forward: 1200 steps, Backward: -1195 steps
[PASS] Motor direction control working correctly

[TEST] Testing motor synchronization...
[SYNC] Left: 980, Right: 1002, Difference: 2.2%
[PASS] Motors synchronized within 3% tolerance

--- All Motor Tests Passed! ---
```

---

## 📝 Notes & Observations

1. **Motor Tuning**:
   - Start with acceleration = 300-500 steps/s²
   - Adjust based on wheel slip and torque requirements
   - Higher acceleration = faster ramp, but more motor slip

2. **Encoder Resolution**:
   - 1070 steps/rev = 0.084 mm per step (with 90mm wheel)
   - Encoder provides sub-millimeter position feedback
   - Drift < 1mm over 1m of travel typical

3. **PWM Deadband**:
   - Most DC motors require PWM > 150 to overcome friction
   - Adjust `updateMotorOutput()` deadband if needed
   - Lower deadband = smoother low-speed control

4. **Real-World Motor Behavior**:
   - Wheel friction affects low-speed response
   - Paper surface may have resistance zones
   - Consider velocity feedforward (add open-loop component to PID)

---

## 🎯 Next Steps

**Phase 1 Complete!**
→ Move to [girobot_step2.md](girobot_step2.md): **IMU Sensor Integration**

In Phase 2, you will:
1. Initialize MPU9250 or BNO055 sensor
2. Calibrate gyro, accelerometer, and magnetometer
3. Implement Mahony sensor fusion filter
4. Test heading stability and accuracy

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate (hands-on testing)  
**Hardware Required**: ESP32, motors, encoders, motor drivers, paper surface  

Last Updated: May 11, 2026
