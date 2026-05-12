# GiRobot Step 10: Map & Ghost Map Display

> **Phase 10**: Implement real-time visualization of robot path, ghost pose (pure odometry), and drift overlay on web dashboard.

---

## 📋 Objectives

1. ✅ Display real robot pose on map canvas
2. ✅ Overlay ghost pose (odometry-only) in different color
3. ✅ Plot pen trace history
4. ✅ Show heading indicator
5. ✅ Visualize path deviation

---

## 📊 Extended Telemetry Format

Modify [src/CommsManager.cpp](../src/CommsManager.cpp) to include trace buffer:

```cpp
void CommsManager::sendTelemetry() {
    DynamicJsonDocument doc(4096);  // Larger buffer for trace data
    
    // ... existing pose, motors, imu, battery data ...
    
    // Ghost pose (pure odometry)
    doc["ghost_pose"]["x"] = pose.getGhostX();
    doc["ghost_pose"]["y"] = pose.getGhostY();
    doc["ghost_pose"]["heading_deg"] = pose.getGhostHeading() * 180 / PI;
    
    // Pen trace (last 50 points)
    const auto& traceBuffer = motion.getTraceBuffer();
    JsonArray traceArray = doc.createNestedArray("trace");
    
    int traceCount = min(50, traceBuffer.getPointCount());
    for (int i = 0; i < traceCount; i++) {
        // Get trace points (implementation depends on buffer structure)
        JsonObject point = traceArray.createNestedObject();
        point["x"] = traceBuffer.getPoint(i).x;
        point["y"] = traceBuffer.getPoint(i).y;
    }
    
    // Drift analysis
    float realDist = sqrt(pose.getPenX()*pose.getPenX() + 
                          pose.getPenY()*pose.getPenY());
    float ghostDist = sqrt(pose.getGhostX()*pose.getGhostX() + 
                           pose.getGhostY()*pose.getGhostY());
    
    doc["drift"]["distance_error_mm"] = abs(realDist - ghostDist);
    doc["drift"]["heading_error_deg"] = 
        abs((pose.getHeading() - pose.getGhostHeading()) * 180 / PI);
    
    String json;
    serializeJson(doc, json);
    server.textAll(json);
}
```

---

## 🎨 Enhanced data/script.js

Update drawMap() method:

```javascript
drawMap() {
    const canvas = document.getElementById('mapCanvas');
    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const scale = 0.5;  // pixels per mm
    
    // Clear canvas
    ctx.fillStyle = '#0f3460';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Draw grid
    ctx.strokeStyle = '#1a3a52';
    ctx.lineWidth = 1;
    for (let i = -400; i <= 400; i += 100) {
        ctx.beginPath();
        ctx.moveTo(centerX + i * scale, 0);
        ctx.lineTo(centerX + i * scale, canvas.height);
        ctx.stroke();
        
        ctx.beginPath();
        ctx.moveTo(0, centerY + i * scale);
        ctx.lineTo(canvas.width, centerY + i * scale);
        ctx.stroke();
    }
    
    // Draw pen trace history
    if (this.telemetry.trace && this.telemetry.trace.length > 1) {
        ctx.strokeStyle = '#4caf50';
        ctx.lineWidth = 2;
        ctx.beginPath();
        
        const firstPoint = this.telemetry.trace[0];
        ctx.moveTo(centerX + firstPoint.x * scale, centerY + firstPoint.y * scale);
        
        for (let i = 1; i < this.telemetry.trace.length; i++) {
            const point = this.telemetry.trace[i];
            ctx.lineTo(centerX + point.x * scale, centerY + point.y * scale);
        }
        
        ctx.stroke();
    }
    
    // Draw ghost pose (odometry-only) in dim color
    if (this.telemetry.ghost_pose) {
        const gx = centerX + this.telemetry.ghost_pose.x * scale;
        const gy = centerY + this.telemetry.ghost_pose.y * scale;
        const gHeading = this.telemetry.ghost_pose.heading_deg * Math.PI / 180;
        
        // Ghost robot circle (faded)
        ctx.fillStyle = 'rgba(102, 126, 234, 0.3)';
        ctx.beginPath();
        ctx.arc(gx, gy, 4, 0, 2 * Math.PI);
        ctx.fill();
        
        // Ghost heading indicator
        ctx.strokeStyle = 'rgba(102, 126, 234, 0.5)';
        ctx.lineWidth = 1;
        ctx.setLineDash([3, 3]);
        ctx.beginPath();
        ctx.moveTo(gx, gy);
        ctx.lineTo(gx + 12 * Math.cos(gHeading), gy + 12 * Math.sin(gHeading));
        ctx.stroke();
        ctx.setLineDash([]);
    }
    
    // Draw real pose (bright color)
    if (this.telemetry.pose) {
        const x = centerX + this.telemetry.pose.x * scale;
        const y = centerY + this.telemetry.pose.y * scale;
        const heading = this.telemetry.pose.heading_deg * Math.PI / 180;
        
        // Real robot body
        ctx.fillStyle = '#667eea';
        ctx.beginPath();
        ctx.arc(x, y, 6, 0, 2 * Math.PI);
        ctx.fill();
        
        // Real robot heading indicator
        ctx.strokeStyle = '#667eea';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(x, y);
        ctx.lineTo(x + 18 * Math.cos(heading), y + 18 * Math.sin(heading));
        ctx.stroke();
        
        // Add small triangle for direction
        ctx.fillStyle = '#667eea';
        const tipX = x + 18 * Math.cos(heading);
        const tipY = y + 18 * Math.sin(heading);
        ctx.beginPath();
        ctx.moveTo(tipX, tipY);
        ctx.lineTo(tipX - 3 * Math.cos(heading + 2.3), 
                   tipY - 3 * Math.sin(heading + 2.3));
        ctx.lineTo(tipX - 3 * Math.cos(heading - 2.3), 
                   tipY - 3 * Math.sin(heading - 2.3));
        ctx.closePath();
        ctx.fill();
    }
    
    // Draw origin marker
    ctx.fillStyle = '#888';
    ctx.fillRect(centerX - 3, centerY - 3, 6, 6);
    
    // Draw axes labels
    ctx.fillStyle = '#666';
    ctx.font = 'bold 12px Arial';
    ctx.fillText('0', centerX - 10, centerY - 10);
    ctx.fillText('Y(mm)', 5, 15);
    ctx.fillText('X(mm)', canvas.width - 40, centerY + 15);
}
```

Add drift display to telemetry panel (index.html):

```html
<div class="telemetry-group">
    <h3>📍 Position Accuracy</h3>
    <p>Real: <span id="realDist">0</span> mm from origin</p>
    <p>Ghost: <span id="ghostDist">0</span> mm from origin</p>
    <p>Drift Error: <span id="driftError">0</span> mm</p>
</div>
```

Update telemetry in script.js:

```javascript
updateUI() {
    // ... existing updates ...
    
    if (this.telemetry.drift) {
        document.getElementById('driftError').textContent = 
            this.telemetry.drift.distance_error_mm.toFixed(1);
    }
}
```

---

## ✅ Verification Checklist: Step 10

- [ ] **Trace Display**: Green line shows robot path history
- [ ] **Ghost Pose**: Dim dashed indicator shows odometry-only path
- [ ] **Real Pose**: Bright arrow shows actual pose with IMU fusion
- [ ] **Drift Visualization**: Visual separation shows IMU correction
- [ ] **Origin Marker**: (0,0) marked on map
- [ ] **Heading Indicator**: Robot direction shown as arrow
- [ ] **Map Updates**: Real-time position updates on canvas

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate  
**Next**: [girobot_step11.md](girobot_step11.md)

Last Updated: May 11, 2026
