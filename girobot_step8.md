# GiRobot Step 8: Web Frontend (HTML/CSS/JS Creation)

> **Phase 8**: Create responsive web dashboard with real-time telemetry display, WebSocket client, and robot control interface.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Create index.html with dashboard layout
2. ✅ Create style.css with responsive design
3. ✅ Create script.js with WebSocket client
4. ✅ Implement real-time telemetry plotting
5. ✅ Build control interface (joystick, buttons)
6. ✅ Test in browser at http://192.168.4.1

---

## 📄 data/index.html

Create dashboard layout with 3-column design:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GiRobot Dashboard</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <!-- Header -->
        <header>
            <h1>🤖 GiRobot Control Dashboard</h1>
            <div id="status" class="status disconnected">Disconnected</div>
        </header>

        <!-- Main Content -->
        <div class="main-grid">
            <!-- Left Panel: Controls -->
            <div class="panel controls-panel">
                <h2>Controls</h2>
                
                <!-- Joystick -->
                <div class="joystick-container">
                    <h3>Motion Control</h3>
                    <canvas id="joystick" width="200" height="200"></canvas>
                    <p><small>Click and drag to control linear/angular velocity</small></p>
                </div>

                <!-- Buttons -->
                <div class="button-group">
                    <button onclick="robot.stop()">🛑 STOP</button>
                    <button onclick="robot.calibrateIMU()">⚙️ Calibrate</button>
                </div>

                <!-- Distance Commands -->
                <div class="command-group">
                    <h3>Distance Commands</h3>
                    <input type="number" id="distance" placeholder="Distance (mm)" value="200">
                    <button onclick="robot.goStraight()">→ Go Straight</button>
                    <input type="number" id="angle" placeholder="Angle (deg)" value="90">
                    <button onclick="robot.rotate()">↻ Rotate</button>
                </div>
            </div>

            <!-- Center Panel: Robot Visualization -->
            <div class="panel visualization-panel">
                <h2>Robot Position Map</h2>
                <canvas id="mapCanvas" width="400" height="400"></canvas>
                <div id="mapInfo">
                    <p>X: <span id="mapX">0</span> mm | Y: <span id="mapY">0</span> mm | θ: <span id="mapTheta">0</span>°</p>
                </div>
            </div>

            <!-- Right Panel: Telemetry & Drawing -->
            <div class="panel telemetry-panel">
                <h2>Telemetry</h2>
                
                <!-- IMU Data -->
                <div class="telemetry-group">
                    <h3>IMU</h3>
                    <p>Heading: <span id="imuHeading">0.0</span>°</p>
                    <p>Pitch: <span id="imuPitch">0.0</span>°</p>
                    <p>Roll: <span id="imuRoll">0.0</span>°</p>
                </div>

                <!-- Motor Data -->
                <div class="telemetry-group">
                    <h3>Motors</h3>
                    <p>Left: <span id="motorLeft">0.0</span> mm/s</p>
                    <p>Right: <span id="motorRight">0.0</span> mm/s</p>
                </div>

                <!-- Battery -->
                <div class="telemetry-group">
                    <h3>Power</h3>
                    <p>Voltage: <span id="batteryV">0.0</span> V</p>
                    <p>Level: <span id="batteryPercent">0</span>%</p>
                </div>

                <!-- Drawing Commands -->
                <div class="command-group">
                    <h3>Draw Commands</h3>
                    <button onclick="robot.drawCircle()">⭕ Circle</button>
                    <button onclick="robot.drawTriangle()">△ Triangle</button>
                    <input type="text" id="drawText" placeholder="Text to write">
                    <button onclick="robot.drawText()">✏️ Write</button>
                </div>
            </div>
        </div>

        <!-- Footer -->
        <footer>
            <p>GiRobot v0.1.1 | Connected to 192.168.4.1</p>
        </footer>
    </div>

    <script src="script.js"></script>
</body>
</html>
```

---

## 🎨 data/style.css

Create responsive styling:

```css
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: #1a1a2e;
    color: #eee;
    line-height: 1.6;
}

.container {
    max-width: 1600px;
    margin: 0 auto;
    padding: 20px;
}

header {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    padding: 20px;
    border-radius: 8px;
    margin-bottom: 20px;
    display: flex;
    justify-content: space-between;
    align-items: center;
}

header h1 {
    font-size: 28px;
    font-weight: bold;
}

.status {
    padding: 8px 16px;
    border-radius: 20px;
    font-weight: bold;
    font-size: 14px;
}

.status.connected {
    background: #4caf50;
    color: white;
}

.status.disconnected {
    background: #f44336;
    color: white;
}

.main-grid {
    display: grid;
    grid-template-columns: 1fr 1.2fr 1fr;
    gap: 20px;
    margin-bottom: 20px;
}

@media (max-width: 1200px) {
    .main-grid {
        grid-template-columns: 1fr;
    }
}

.panel {
    background: #16213e;
    border: 2px solid #667eea;
    border-radius: 8px;
    padding: 20px;
    box-shadow: 0 8px 16px rgba(0,0,0,0.3);
}

.panel h2 {
    margin-bottom: 15px;
    color: #667eea;
    border-bottom: 2px solid #667eea;
    padding-bottom: 10px;
}

.panel h3 {
    margin-top: 15px;
    margin-bottom: 10px;
    color: #aaa;
    font-size: 14px;
}

/* Controls Panel */
.joystick-container {
    margin-bottom: 20px;
}

canvas#joystick {
    width: 100%;
    border: 2px solid #667eea;
    border-radius: 50%;
    background: #0f3460;
    cursor: crosshair;
}

.button-group, .command-group {
    margin: 15px 0;
}

button {
    width: 100%;
    padding: 10px;
    margin: 5px 0;
    background: #667eea;
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
    font-weight: bold;
    font-size: 14px;
    transition: background 0.3s;
}

button:hover {
    background: #764ba2;
}

button:active {
    transform: scale(0.98);
}

input[type="number"],
input[type="text"] {
    width: 100%;
    padding: 8px;
    margin: 5px 0;
    background: #0f3460;
    border: 1px solid #667eea;
    border-radius: 4px;
    color: #eee;
    font-size: 14px;
}

/* Visualization Panel */
#mapCanvas {
    width: 100%;
    max-width: 500px;
    border: 2px solid #667eea;
    border-radius: 8px;
    background: #0f3460;
    display: block;
    margin: 0 auto 10px;
}

#mapInfo {
    text-align: center;
    font-size: 12px;
    color: #aaa;
}

/* Telemetry Panel */
.telemetry-group {
    margin: 15px 0;
    padding: 10px;
    background: #0f3460;
    border-left: 3px solid #667eea;
    border-radius: 4px;
}

.telemetry-group p {
    font-size: 13px;
    margin: 5px 0;
    color: #ddd;
}

.telemetry-group span {
    color: #667eea;
    font-weight: bold;
}

/* Footer */
footer {
    text-align: center;
    padding: 15px;
    color: #888;
    font-size: 12px;
    border-top: 1px solid #667eea;
}

/* Responsive */
@media (max-width: 768px) {
    header {
        flex-direction: column;
        gap: 10px;
    }
    header h1 {
        font-size: 20px;
    }
}
```

---

## 🔌 data/script.js

Create WebSocket client and control logic:

```javascript
class GiRobotClient {
    constructor() {
        this.ws = null;
        this.telemetry = {};
        this.joystick = {
            linear: 0,
            angular: 0,
            isActive: false
        };
        this.setupCanvas();
        this.connect();
    }

    connect() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}:81`;
        
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('Connected to robot');
            this.updateStatus(true);
        };
        
        this.ws.onmessage = (e) => {
            try {
                this.telemetry = JSON.parse(e.data);
                this.updateUI();
            } catch (err) {
                console.error('JSON parse error:', err);
            }
        };
        
        this.ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            this.updateStatus(false);
        };
        
        this.ws.onclose = () => {
            console.log('Disconnected from robot');
            this.updateStatus(false);
            setTimeout(() => this.connect(), 3000);
        };
    }

    send(command) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(command));
        }
    }

    setupCanvas() {
        const canvas = document.getElementById('joystick');
        const ctx = canvas.getContext('2d');
        
        // Draw circle
        ctx.fillStyle = '#667eea';
        ctx.beginPath();
        ctx.arc(100, 100, 90, 0, 2 * Math.PI);
        ctx.fill();
        
        // Draw center point
        ctx.fillStyle = '#0f3460';
        ctx.beginPath();
        ctx.arc(100, 100, 80, 0, 2 * Math.PI);
        ctx.fill();
        
        // Joystick control
        canvas.addEventListener('mousedown', (e) => {
            this.joystick.isActive = true;
            this.updateJoystick(e);
        });
        
        canvas.addEventListener('mousemove', (e) => {
            if (this.joystick.isActive) {
                this.updateJoystick(e);
            }
        });
        
        canvas.addEventListener('mouseup', () => {
            this.joystick.isActive = false;
            this.joystick.linear = 0;
            this.joystick.angular = 0;
            this.send({cmd: 'DRIVE', linear_mm_s: 0, angular_rad_s: 0});
        });
    }

    updateJoystick(e) {
        const rect = e.target.getBoundingClientRect();
        const x = e.clientX - rect.left - 100;
        const y = e.clientY - rect.top - 100;
        
        const distance = Math.sqrt(x*x + y*y);
        const angle = Math.atan2(x, -y);
        
        if (distance < 80) {
            this.joystick.linear = (distance / 80) * 100;  // -100 to +100 mm/s
            this.joystick.angular = angle;  // -π to +π rad/s
            
            this.send({
                cmd: 'DRIVE',
                linear_mm_s: this.joystick.linear,
                angular_rad_s: this.joystick.angular * 0.5
            });
        }
    }

    stop() {
        this.send({cmd: 'STOP'});
    }

    goStraight() {
        const distance = parseFloat(document.getElementById('distance').value) || 200;
        this.send({cmd: 'GO_STRAIGHT', distance_mm: distance});
    }

    rotate() {
        const angle = parseFloat(document.getElementById('angle').value) || 90;
        this.send({cmd: 'ROTATE', angle_deg: angle});
    }

    drawCircle() {
        this.send({cmd: 'DRAW_CIRCLE', x: 0, y: 0, radius: 50});
    }

    drawTriangle() {
        this.send({cmd: 'DRAW_TRIANGLE', x1: 0, y1: 50, x2: -50, y2: -50, x3: 50, y3: -50});
    }

    drawText() {
        const text = document.getElementById('drawText').value || 'HELLO';
        this.send({cmd: 'DRAW_TEXT', text: text, x: 0, y: 0, size: 20});
    }

    calibrateIMU() {
        this.send({cmd: 'CALIBRATE_IMU'});
    }

    updateUI() {
        // Telemetry updates
        if (this.telemetry.imu) {
            document.getElementById('imuHeading').textContent = 
                this.telemetry.imu.heading_deg.toFixed(1);
            document.getElementById('imuPitch').textContent = 
                this.telemetry.imu.pitch_deg.toFixed(1);
            document.getElementById('imuRoll').textContent = 
                this.telemetry.imu.roll_deg.toFixed(1);
        }
        
        if (this.telemetry.motors) {
            document.getElementById('motorLeft').textContent = 
                this.telemetry.motors.left_mm_s.toFixed(1);
            document.getElementById('motorRight').textContent = 
                this.telemetry.motors.right_mm_s.toFixed(1);
        }
        
        if (this.telemetry.battery) {
            document.getElementById('batteryV').textContent = 
                this.telemetry.battery.voltage_v.toFixed(1);
            document.getElementById('batteryPercent').textContent = 
                this.telemetry.battery.percent.toFixed(0);
        }
        
        if (this.telemetry.pose) {
            document.getElementById('mapX').textContent = 
                this.telemetry.pose.x.toFixed(1);
            document.getElementById('mapY').textContent = 
                this.telemetry.pose.y.toFixed(1);
            document.getElementById('mapTheta').textContent = 
                this.telemetry.pose.heading_deg.toFixed(1);
            
            this.drawMap();
        }
    }

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
        ctx.strokeStyle = '#333';
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
        
        // Draw robot
        if (this.telemetry.pose) {
            const x = centerX + this.telemetry.pose.x * scale;
            const y = centerY + this.telemetry.pose.y * scale;
            const heading = this.telemetry.pose.heading_deg * Math.PI / 180;
            
            // Robot body
            ctx.fillStyle = '#667eea';
            ctx.beginPath();
            ctx.arc(x, y, 5, 0, 2 * Math.PI);
            ctx.fill();
            
            // Heading indicator
            ctx.strokeStyle = '#667eea';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(x, y);
            ctx.lineTo(x + 15 * Math.cos(heading), y + 15 * Math.sin(heading));
            ctx.stroke();
        }
        
        // Draw origin
        ctx.fillStyle = '#888';
        ctx.fillRect(centerX - 2, centerY - 2, 4, 4);
    }

    updateStatus(connected) {
        const status = document.getElementById('status');
        if (connected) {
            status.textContent = '✓ Connected';
            status.className = 'status connected';
        } else {
            status.textContent = '✗ Disconnected';
            status.className = 'status disconnected';
        }
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    window.robot = new GiRobotClient();
});
```

---

## ✅ Verification Checklist: Step 8

- [ ] **Files Created**: index.html, style.css, script.js in `/data/` folder
- [ ] **Dashboard Loads**: http://192.168.4.1 displays without errors
- [ ] **Status Indicator**: Shows "Connected" when WebSocket active
- [ ] **Telemetry Display**: All fields update every 100-200ms
- [ ] **Joystick**: Responds to mouse drag, sends DRIVE commands
- [ ] **Robot Map**: Displays robot position with heading indicator
- [ ] **Buttons Functional**: STOP, Calibrate, Distance commands work
- [ ] **Drawing Commands**: Draw buttons send correct JSON

---

## 🎯 Next Steps

**Phase 8 Complete!**
→ Move to [girobot_step9.md](girobot_step9.md): **Web Controls & Extended Commands**

---

**Estimated Time**: 3 hours  
**Difficulty**: ⭐⭐ Intermediate (web development)  
**Tools Required**: Browser, text editor  

Last Updated: May 11, 2026
