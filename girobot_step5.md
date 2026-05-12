# GiRobot Step 5: Pen Features & Drawing Logic

> **Phase 5**: Implement pen dynamics, contact detection, trace buffering, and drawing command handlers for geometric shapes and text.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand pen-centric geometry (130 mm offset from axle)
2. ✅ Implement pen contact detection (optional lift logic)
3. ✅ Buffer pen trace points for visualization
4. ✅ Create drawing command handlers
5. ✅ Test line drawing accuracy (< 2mm deviation)
6. ✅ Validate circle drawing (radius accuracy)
7. ✅ Test text rendering (character size and spacing)

---

## 🔧 Pen Dynamics & Geometry

### Pen Offset Physics

The pen tip is **130 mm ahead** of the wheel axle center:

```
        Pen (0, 0) ← Origin when x=0, y=0
         /    \
        /      \
       /        \
  L.Wheel --- R.Wheel
  [-D, -L/2] [-D, +L/2]

where:
  D = 130 mm (pen offset)
  L = 83 mm (wheel base)
```

### Trajectory Correction

When the robot moves and turns, the pen traces a different path than the wheel axle:

```
Desired path (pen tip):     [x_pen, y_pen]
Wheel path (axle center):   [x_axle, y_axle]

Relationship:
x_pen = x_axle + D × cos(θ)
y_pen = y_axle + D × sin(θ)

When turning:
ω = angular velocity
V_left = V_linear - (ω × L/2)
V_right = V_linear + (ω × L/2)

Path planning must account for this offset to keep pen on desired trajectory.
```

---

## 📖 Implementing Pen Features

### 1. Pen Trace Buffering

Create circular buffer to store last N pen positions for visualization:

```cpp
// In MotionController.h
class PenTraceBuffer {
private:
    static constexpr int BUFFER_SIZE = 5000;  // Store 5000 points
    
    struct TracePoint {
        float x, y;           // mm
        uint32_t timestamp;   // ms
    };
    
    TracePoint buffer[BUFFER_SIZE];
    int writeIndex = 0;
    int pointCount = 0;
    
public:
    void addPoint(float x, float y) {
        buffer[writeIndex] = {x, y, millis()};
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;
        
        if (pointCount < BUFFER_SIZE) {
            pointCount++;
        }
    }
    
    int getPointCount() const {
        return pointCount;
    }
    
    const TracePoint* getBuffer() const {
        return buffer;
    }
    
    int getStartIndex() const {
        // If buffer full, start from oldest point
        return (pointCount == BUFFER_SIZE) ? writeIndex : 0;
    }
    
    void clear() {
        writeIndex = 0;
        pointCount = 0;
    }
};
```

Add to MotionController:

```cpp
void MotionController::update() {
    // ... existing motion control code ...
    
    // Always buffer current pen position
    traceBuffer.addPoint(pose.getPenX(), pose.getPenY());
}
```

### 2. Pen Contact Detection

Optional: detect when pen loses contact with paper:

```cpp
class PenContactDetector {
private:
    // Could integrate pressure sensor reading here
    // For now, simple model: pen always in contact
    
public:
    bool isPenDown() const {
        return true;  // Always down per project spec
    }
    
    // Future expansion: 
    // - Digital contact sensor (GPIO)
    // - Pressure sensor (ADC)
    // - Current draw sensing from motor
};
```

### 3. Drawing Command Handlers

In [src/MotionController.cpp](../src/MotionController.cpp), add:

```cpp
void MotionController::drawLine(float x1, float y1, float x2, float y2) {
    // Generate waypoints along line
    // Distance between waypoints: 5 mm (for smooth curve on paper)
    
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrt(dx*dx + dy*dy);
    
    int numSegments = (int)(distance / 5.0f) + 1;  // 5 mm segments
    
    // Clear previous path
    clearWaypoints();
    
    for (int i = 0; i <= numSegments; i++) {
        float t = (float)i / numSegments;
        float x = x1 + dx * t;
        float y = y1 + dy * t;
        
        addWaypoint(x, y);
    }
    
    setMode(Mode::AUTO_PATH);
}

void MotionController::drawCircle(float centerX, float centerY, float radius) {
    // Generate waypoints around circle perimeter
    // Use PathPlanner to generate the path
    
    clearWaypoints();
    
    Vector<MotionWaypoint> circlePath = pathPlanner.generateCircle(
        centerX, centerY, radius, 
        2.0f  // 2 mm segments for smooth circle
    );
    
    for (const auto& wp : circlePath) {
        addWaypoint(wp.x, wp.y);
    }
    
    setMode(Mode::AUTO_PATH);
}

void MotionController::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    clearWaypoints();
    
    Vector<MotionWaypoint> trianglePath = pathPlanner.generateTriangle(
        x1, y1, x2, y2, x3, y3,
        2.0f  // 2 mm segments
    );
    
    for (const auto& wp : trianglePath) {
        addWaypoint(wp.x, wp.y);
    }
    
    setMode(Mode::AUTO_PATH);
}

void MotionController::drawText(const String& text, float x, float y, float size) {
    clearWaypoints();
    
    // PathPlanner converts text to waypoint path
    Vector<MotionWaypoint> textPath = pathPlanner.generateText(
        text, x, y, size,
        2.0f  // 2 mm segments
    );
    
    for (const auto& wp : textPath) {
        addWaypoint(wp.x, wp.y);
    }
    
    setMode(Mode::AUTO_PATH);
}
```

---

## 🧪 Testing Procedure

### Test 1: Line Drawing Accuracy

**Expected**: Drawn line matches command (< 2 mm deviation).

```cpp
void testLineDrawing() {
    Serial.println("\n[TEST] Line drawing accuracy (100 mm straight)...");
    
    pose.reset();
    motion.drawLine(0, 0, 100, 0);  // Draw 100mm horizontal line
    
    uint32_t startTime = millis();
    float maxDeviation = 0;
    
    while (!motion.isPathComplete() && (millis() - startTime < 15000)) {
        pose.update();
        motion.update();
        
        float penX = pose.getPenX();
        float penY = pose.getPenY();
        
        // For horizontal line, Y should be ~0
        float deviation = abs(penY);
        if (deviation > maxDeviation) maxDeviation = deviation;
        
        if ((millis() - startTime) % 2000 == 0) {
            Serial.printf("[LINE] Time=%ldms, X=%.1f mm, Y=%.1f mm (deviation=%.1f mm)\n",
                          millis() - startTime, penX, penY, deviation);
        }
        
        delay(10);
    }
    
    Serial.printf("[RESULT] Max deviation from line: %.1f mm (target < 2 mm)\n",
                  maxDeviation);
    
    if (maxDeviation < 2.0) {
        Serial.println("[PASS] Line drawing excellent!");
    } else {
        Serial.println("[WARN] Significant line deviation - check PID tuning");
    }
}
```

---

### Test 2: Circle Drawing Accuracy

**Expected**: Drawn circle has consistent radius (± 3 mm), completes in < 30 seconds.

```cpp
void testCircleDrawing() {
    Serial.println("\n[TEST] Circle drawing accuracy (radius=50 mm)...");
    
    pose.reset();
    motion.drawCircle(0, 0, 50.0f);  // Draw circle at origin
    
    uint32_t startTime = millis();
    float minRadius = 999, maxRadius = 0;
    float angleProgress = 0;
    
    while (!motion.isPathComplete() && (millis() - startTime < 40000)) {
        pose.update();
        motion.update();
        
        float penX = pose.getPenX();
        float penY = pose.getPenY();
        
        float radius = sqrt(penX*penX + penY*penY);
        if (radius < minRadius) minRadius = radius;
        if (radius > maxRadius) maxRadius = radius;
        
        float angle = atan2(penY, penX) * 180 / PI;
        
        if ((millis() - startTime) % 5000 == 0) {
            Serial.printf("[CIRCLE] Time=%ldms, Angle=%d°, Radius=%.1f mm\n",
                          millis() - startTime, (int)angle, radius);
        }
        
        delay(10);
    }
    
    float radiusError = (maxRadius - minRadius) / 2.0f;
    float avgRadius = (maxRadius + minRadius) / 2.0f;
    
    Serial.printf("[RESULT] Radius: %.1f mm ± %.1f mm (expected 50 ± 3 mm)\n",
                  avgRadius, radiusError);
    
    if (abs(avgRadius - 50.0f) < 5.0 && radiusError < 3.0) {
        Serial.println("[PASS] Circle drawing excellent!");
    } else {
        Serial.println("[WARN] Circle accuracy needs improvement");
    }
}
```

---

### Test 3: Triangle Drawing

**Expected**: Draws triangle with sharp corners, each side roughly equal length.

```cpp
void testTriangleDrawing() {
    Serial.println("\n[TEST] Triangle drawing...");
    
    pose.reset();
    
    // Equilateral triangle vertices
    float x1 = 0, y1 = 100;      // Top
    float x2 = -86.6f, y2 = -50; // Bottom-left
    float x3 = 86.6f, y3 = -50;  // Bottom-right
    
    motion.drawTriangle(x1, y1, x2, y2, x3, y3);
    
    uint32_t startTime = millis();
    
    while (!motion.isPathComplete() && (millis() - startTime < 30000)) {
        pose.update();
        motion.update();
        delay(10);
    }
    
    Serial.println("[RESULT] Triangle trace complete");
    Serial.println("[PASS] Triangle drawn successfully!");
}
```

---

### Test 4: Text Rendering

**Expected**: Text renders with clear characters, correct spacing.

```cpp
void testTextRendering() {
    Serial.println("\n[TEST] Text rendering (text='HELLO')...");
    
    pose.reset();
    motion.drawText("HELLO", 0, 0, 20.0f);  // 20mm character size
    
    uint32_t startTime = millis();
    
    while (!motion.isPathComplete() && (millis() - startTime < 60000)) {
        pose.update();
        motion.update();
        
        if ((millis() - startTime) % 10000 == 0) {
            Serial.printf("[TEXT] Time=%ldms...\n", millis() - startTime);
        }
        
        delay(10);
    }
    
    Serial.println("[RESULT] Text trace complete");
    Serial.println("[PASS] Text rendered successfully!");
}
```

---

### Test 5: Trace Buffer Visualization

**Expected**: Trace buffer captures all pen positions for web display.

```cpp
void testTraceBuffer() {
    Serial.println("\n[TEST] Trace buffer capture...");
    
    pose.reset();
    motion.getTraceBuffer().clear();
    
    // Draw a square to capture
    motion.drawSquare(0, 0, 100);
    
    uint32_t startTime = millis();
    
    while (!motion.isPathComplete() && (millis() - startTime < 30000)) {
        pose.update();
        motion.update();
        delay(10);
    }
    
    int tracePointCount = motion.getTraceBuffer().getPointCount();
    
    Serial.printf("[RESULT] Captured %d trace points\n", tracePointCount);
    
    if (tracePointCount > 100) {
        Serial.println("[PASS] Trace buffer working!");
    } else {
        Serial.println("[WARN] Low trace point count");
    }
}
```

---

## ✅ Verification Checklist: Step 5

- [ ] **Line Drawing**: < 2 mm deviation from straight line
- [ ] **Circle Drawing**: Radius error < 3 mm, average radius within 5 mm of target
- [ ] **Triangle Drawing**: Completes without path errors
- [ ] **Text Rendering**: Characters legible, proper spacing
- [ ] **Trace Buffer**: Captures > 100 points per drawing operation
- [ ] **No Motion Errors**: All drawing commands complete without faults
- [ ] **Pen Contact**: Contact detection working (always down per spec)

---

## 📊 Expected Telemetry Format

For web dashboard display:

```json
{
  "drawing_state": {
    "mode": "AUTO_PATH",
    "current_command": "DRAW_CIRCLE",
    "command_progress": 45.3,
    "waypoint_index": 23,
    "waypoint_count": 51
  },
  "pen_state": {
    "x": 42.5,
    "y": 28.3,
    "heading_deg": 45.2,
    "contact": true,
    "trace_points": [
      {"x": 0, "y": 0},
      {"x": 1.2, "y": 0.8},
      {"x": 2.5, "y": 1.9},
      ...
    ]
  },
  "path_status": {
    "is_complete": false,
    "cross_track_error_mm": 0.5,
    "distance_to_target": 1.2
  }
}
```

---

## 🎯 Next Steps

**Phase 5 Complete!**
→ Move to [girobot_step6.md](girobot_step6.md): **Geometry & Text Rendering**

In Phase 6, you will:
1. Review PathPlanner.cpp (already implemented)
2. Validate circle/triangle/rectangle generation
3. Test text rendering with Bezier smoothing
4. Document drawing command API

---

**Estimated Time**: 1.5 hours (mostly testing)  
**Difficulty**: ⭐⭐ Intermediate (testing drawing accuracy)  
**Hardware Required**: ESP32, motors, pen, paper  

Last Updated: May 11, 2026
