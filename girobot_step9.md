# GiRobot Step 9: Web Controls & Extended Commands

> **Phase 9**: Extend web interface with geometry drawing controls, text input, and command dispatcher.

---

## 📋 Objectives

1. ✅ Add geometry control buttons (Circle, Triangle, Rectangle)
2. ✅ Add text drawing interface
3. ✅ Implement command parameter input
4. ✅ Test all web-based commands

---

## 🎨 Extend data/index.html

Add to the Drawing Commands section:

```html
<!-- Drawing Commands (Enhanced) -->
<div class="command-group">
    <h3>🎨 Draw Shapes</h3>
    
    <div class="shape-controls">
        <label>Circle:</label>
        <input type="number" id="circleRadius" placeholder="Radius (mm)" value="50">
        <button onclick="robot.drawCircle()">⭕ Draw Circle</button>
        
        <label>Triangle Size:</label>
        <input type="number" id="triangleSize" placeholder="Size (mm)" value="100">
        <button onclick="robot.drawTriangle()">△ Triangle</button>
        
        <label>Rectangle:</label>
        <input type="number" id="rectWidth" placeholder="Width (mm)" value="100">
        <input type="number" id="rectHeight" placeholder="Height (mm)" value="60">
        <button onclick="robot.drawRectangle()">▭ Rectangle</button>
    </div>
    
    <h3>✏️ Write Text</h3>
    <div class="text-controls">
        <input type="text" id="drawText" placeholder="Text to write" value="HELLO" maxlength="20">
        <input type="number" id="fontSize" placeholder="Font size (mm)" value="20">
        <button onclick="robot.drawText()">✏️ Write Text</button>
    </div>
    
    <h3>📍 Custom Position</h3>
    <input type="number" id="posX" placeholder="X position (mm)" value="0">
    <input type="number" id="posY" placeholder="Y position (mm)" value="0">
    <button onclick="robot.moveToPosition()">⬆️ Go To Position</button>
</div>
```

---

## 🔌 Extend data/script.js

Add new command methods:

```javascript
// In GiRobotClient class:

drawCircle() {
    const radius = parseFloat(document.getElementById('circleRadius').value) || 50;
    const x = parseFloat(document.getElementById('posX').value) || 0;
    const y = parseFloat(document.getElementById('posY').value) || 0;
    
    this.send({
        cmd: 'DRAW_CIRCLE',
        x: x,
        y: y,
        radius: radius
    });
    console.log(`Drawing circle at (${x}, ${y}) with radius ${radius}mm`);
}

drawTriangle() {
    const size = parseFloat(document.getElementById('triangleSize').value) || 100;
    const x = parseFloat(document.getElementById('posX').value) || 0;
    const y = parseFloat(document.getElementById('posY').value) || 0;
    
    // Equilateral triangle vertices
    const h = size * 0.866;  // height = size * sqrt(3)/2
    
    this.send({
        cmd: 'DRAW_TRIANGLE',
        x1: x, y1: y + h/2,
        x2: x - size/2, y2: y - h/2,
        x3: x + size/2, y3: y - h/2
    });
    console.log(`Drawing triangle size ${size}mm at (${x}, ${y})`);
}

drawRectangle() {
    const width = parseFloat(document.getElementById('rectWidth').value) || 100;
    const height = parseFloat(document.getElementById('rectHeight').value) || 60;
    const x = parseFloat(document.getElementById('posX').value) || 0;
    const y = parseFloat(document.getElementById('posY').value) || 0;
    
    this.send({
        cmd: 'DRAW_RECTANGLE',
        x: x,
        y: y,
        width: width,
        height: height
    });
    console.log(`Drawing rectangle ${width}x${height}mm at (${x}, ${y})`);
}

drawText() {
    const text = document.getElementById('drawText').value || 'HELLO';
    const size = parseFloat(document.getElementById('fontSize').value) || 20;
    const x = parseFloat(document.getElementById('posX').value) || 0;
    const y = parseFloat(document.getElementById('posY').value) || 0;
    
    this.send({
        cmd: 'DRAW_TEXT',
        text: text,
        x: x,
        y: y,
        size: size
    });
    console.log(`Writing "${text}" at (${x}, ${y}) size ${size}mm`);
}

moveToPosition() {
    const x = parseFloat(document.getElementById('posX').value) || 0;
    const y = parseFloat(document.getElementById('posY').value) || 0;
    
    this.send({
        cmd: 'MOVE_TO_POSITION',
        x: x,
        y: y
    });
    console.log(`Moving to position (${x}, ${y})`);
}

// Quick presets
drawPreset(shape) {
    const presets = {
        'square': () => {
            document.getElementById('rectWidth').value = '100';
            document.getElementById('rectHeight').value = '100';
            this.drawRectangle();
        },
        'small_circle': () => {
            document.getElementById('circleRadius').value = '30';
            this.drawCircle();
        },
        'large_circle': () => {
            document.getElementById('circleRadius').value = '80';
            this.drawCircle();
        }
    };
    
    if (presets[shape]) presets[shape]();
}
```

---

## 📝 Backend Command Handlers

In [src/CommsManager.cpp](../src/CommsManager.cpp), add handlers:

```cpp
void CommsManager::handleCommand(const char* cmd, const JsonDocument& params) {
    // ... existing handlers ...
    
    else if (strcmp(cmd, "DRAW_RECTANGLE") == 0) {
        float x = params["x"] | 0;
        float y = params["y"] | 0;
        float width = params["width"] | 100;
        float height = params["height"] | 60;
        motion.drawRectangle(x, y, width, height);
    }
    else if (strcmp(cmd, "MOVE_TO_POSITION") == 0) {
        float x = params["x"] | 0;
        float y = params["y"] | 0;
        motion.moveToPosition(x, y);
    }
    else if (strcmp(cmd, "CALIBRATE_IMU") == 0) {
        nav.calibrateGyro();
        delay(1000);
        nav.calibrateMagnetometer();
    }
}
```

---

## ✅ Verification Checklist: Step 9

- [ ] **Drawing Controls Load**: All shape buttons visible in web UI
- [ ] **Circle Drawing**: Web button sends correct JSON with radius
- [ ] **Triangle Drawing**: Button sends 3 vertices
- [ ] **Text Input**: Text field accepts alphanumeric input
- [ ] **Position Input**: X/Y fields update shape location
- [ ] **Commands Execute**: Robot responds to all web commands
- [ ] **Progress Display**: Telemetry shows command execution status

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate  
**Next**: [girobot_step10.md](girobot_step10.md)

Last Updated: May 11, 2026
