# GiRobot Step 7: WebSocket Backend Enhancement

> **Phase 7**: Verify CommsManager.cpp, set up WiFi AP, implement WebSocket server, and test JSON command/telemetry protocol.

---

## 📋 Objectives

By the end of this phase, you will:
1. ✅ Understand CommsManager.cpp (already mostly implemented)
2. ✅ Verify WiFi AP broadcasting ("RobotWifi")
3. ✅ Test WebSocket server connectivity on Port 80
4. ✅ Verify JSON telemetry streaming
5. ✅ Implement command handlers for drawing operations
6. ✅ Test JSON command parsing and execution
7. ✅ Validate WebSocket message format

---

## 🔌 Hardware Configuration

From [include/Config.h](../include/Config.h):

```cpp
constexpr char WifiSsid[] = "RobotWifi";
constexpr char WifiPassword[] = "penbot123";
constexpr char WifiApIp[] = "192.168.4.1";
```

**Connection Steps**:
1. Connect to WiFi network "RobotWifi"
2. Password: `penbot123`
3. Access web server at `http://192.168.4.1`
4. WebSocket connection: `ws://192.168.4.1/ws`

---

## 📖 Understanding CommsManager.cpp

Review [src/CommsManager.cpp](../src/CommsManager.cpp).

### Key Features (Mostly Implemented)

#### 1. **WiFi AP Setup**
```cpp
void CommsManager::begin() {
    // Start WiFi in Access Point (AP) mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Defaults::WifiSsid, Defaults::WifiPassword);
    
    // Set static IP
    IPAddress IP(192, 168, 4, 1);
    IPAddress Gateway(192, 168, 4, 1);
    IPAddress Subnet(255, 255, 255, 0);
    WiFi.softAPConfig(IP, Gateway, Subnet);
    
    Serial.print("[WiFi] AP started: ");
    Serial.println(Defaults::WifiSsid);
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Serve static files from LittleFS
    // GET / → index.html
    // GET /style.css → style.css
    // GET /script.js → script.js
    
    // WebSocket endpoint at /ws
    server.addHandler(new AsyncWebSocketHandler("/ws", 
                                                  onWSEvent));
}
```

#### 2. **WebSocket Event Handler**
```cpp
void CommsManager::onWSEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                             AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client connected: %u\n", client->id());
    }
    else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client disconnected: %u\n", client->id());
    }
    else if (type == WS_EVT_DATA) {
        // Parse incoming JSON command
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, data, len);
        
        const char* cmd = doc["cmd"];
        
        // Route to appropriate handler
        handleCommand(cmd, doc);
    }
}
```

#### 3. **JSON Command Parsing**
Commands arrive as JSON:

```json
{
  "cmd": "DRIVE",
  "linear_mm_s": 50.0,
  "angular_rad_s": 0.0
}
```

```cpp
void CommsManager::handleCommand(const char* cmd, const JsonDocument& params) {
    if (strcmp(cmd, "DRIVE") == 0) {
        float linear = params["linear_mm_s"] | 0.0f;
        float angular = params["angular_rad_s"] | 0.0f;
        motion.drive(linear, angular);
    }
    else if (strcmp(cmd, "STOP") == 0) {
        motion.stop();
    }
    else if (strcmp(cmd, "DRAW_CIRCLE") == 0) {
        float x = params["x"] | 0.0f;
        float y = params["y"] | 0.0f;
        float radius = params["radius"] | 50.0f;
        motion.drawCircle(x, y, radius);
    }
    // ... more handlers ...
}
```

#### 4. **Telemetry Streaming**
```cpp
void CommsManager::sendTelemetry() {
    DynamicJsonDocument doc(1024);
    
    // Pose
    doc["pose"]["x"] = pose.getPenX();
    doc["pose"]["y"] = pose.getPenY();
    doc["pose"]["heading_deg"] = pose.getHeading() * 180 / PI;
    
    // Motor speeds
    doc["motors"]["left_mm_s"] = drive.getLeftSpeed();
    doc["motors"]["right_mm_s"] = drive.getRightSpeed();
    
    // IMU data
    doc["imu"]["heading_deg"] = nav.getYaw();
    doc["imu"]["pitch_deg"] = nav.getPitch();
    doc["imu"]["roll_deg"] = nav.getRoll();
    
    // System status
    doc["battery_v"] = battery.getVoltage();
    doc["battery_percent"] = battery.getPercentage();
    
    // Serialize and send
    String json;
    serializeJson(doc, json);
    server.textAll(json);  // Send to all connected clients
}
```

---

## 🧪 Testing Procedure

### Test 1: WiFi AP Connectivity

**Expected**: ESP32 broadcasts "RobotWifi" network.

**Steps**:
1. Power on ESP32
2. Open WiFi networks list on PC/phone
3. Look for "RobotWifi" network
4. Connect with password "penbot123"

**Expected Result**:
```
[WiFi] AP started: RobotWifi
[WiFi] IP: 192.168.4.1
[WiFi] Clients connected: 0
```

**Troubleshooting**:
- [ ] Network not appearing: Check Config.h WiFi constants
- [ ] Can't connect: Verify password is exactly "penbot123" (8+ chars required)
- [ ] Wrong IP address: Check WiFi.softAPConfig() parameters

---

### Test 2: Web Server Accessibility

**Expected**: HTTP server responds at http://192.168.4.1

**Steps**:
1. Connect to WiFi (previous test)
2. Open browser, navigate to `http://192.168.4.1`
3. Look for web dashboard (in Phase 8, this will be index.html)

**For now** (Phase 7 backend only):
- [ ] Server responds (no 404 error)
- [ ] HTTP headers received
- [ ] (Phase 8 will add actual dashboard HTML)

**Test with curl**:
```bash
curl http://192.168.4.1/
```

**Expected Response**:
```
HTTP/1.1 200 OK
Content-Type: text/html
...
(HTML content or "Not Found" if no index.html yet)
```

---

### Test 3: WebSocket Connection

**Expected**: WebSocket server accepts connections at ws://192.168.4.1/ws

**Test with browser console**:
```javascript
let ws = new WebSocket('ws://192.168.4.1/ws');
ws.onopen = () => console.log('Connected!');
ws.onmessage = (e) => console.log('Received:', e.data);
```

**Expected Serial Output**:
```
[WS] Client connected: 1
```

**Expected Browser Console**:
```
Connected!
```

---

### Test 4: Telemetry Streaming

**Expected**: WebSocket sends JSON telemetry every 100ms.

**Browser console test**:
```javascript
let ws = new WebSocket('ws://192.168.4.1/ws');
ws.onmessage = (e) => {
    let data = JSON.parse(e.data);
    console.log('Pose:', data.pose);
    console.log('Battery:', data.battery_v, 'V');
};
```

**Expected Output**:
```javascript
Pose: {x: 0.0, y: 0.0, heading_deg: 0.0}
Battery: 7.8 V
```

**Serial monitor should show**:
```
[TELEM] Sent telemetry (856 bytes)
[TELEM] Sent telemetry (856 bytes)
...
```

**Test Frequency**:
- [ ] Telemetry updates every 100-200ms
- [ ] No JSON parse errors
- [ ] All fields present

---

### Test 5: Command Reception

**Expected**: Robot executes commands received via WebSocket.

**Browser console test**:
```javascript
let ws = new WebSocket('ws://192.168.4.1/ws');

// Wait for connection
ws.onopen = () => {
    // Send DRIVE command
    ws.send(JSON.stringify({
        cmd: 'DRIVE',
        linear_mm_s: 50.0,
        angular_rad_s: 0.0
    }));
};
```

**Expected**:
- [ ] Serial shows command received: `[CMD] DRIVE: linear=50, angular=0`
- [ ] Robot starts moving forward
- [ ] Telemetry shows non-zero motor speeds

**Serial Output**:
```
[WS] Received command: DRIVE
[CMD] linear_speed: 50.0 mm/s
[CMD] angular_speed: 0.0 rad/s
[MOTION] Motors set to (50.0, 50.0) mm/s
[TELEM] Left: 50.1 mm/s, Right: 49.8 mm/s
```

---

### Test 6: Drawing Commands

**Expected**: Drawing commands parse and execute.

**Browser console**:
```javascript
ws.send(JSON.stringify({
    cmd: 'DRAW_CIRCLE',
    x: 0,
    y: 0,
    radius: 50
}));
```

**Expected**:
- [ ] Serial shows: `[CMD] DRAW_CIRCLE: x=0, y=0, r=50`
- [ ] Robot starts following circle path
- [ ] Telemetry updates with path progress

**Serial Output**:
```
[WS] Received command: DRAW_CIRCLE
[CMD] Parsed parameters: x=0, y=0, radius=50
[MOTION] Mode changed to AUTO_PATH
[PATH] 157 waypoints loaded
[PATH] Starting circle trace...
[TELEM] Waypoint 1/157, Progress=0.6%
```

---

### Test 7: Error Handling

**Expected**: Invalid JSON or bad parameters handled gracefully.

**Send invalid command**:
```javascript
ws.send(JSON.stringify({
    cmd: 'INVALID_COMMAND',
    param: 'value'
}));
```

**Expected**:
- [ ] Serial shows error: `[ERROR] Unknown command: INVALID_COMMAND`
- [ ] Robot doesn't crash
- [ ] WebSocket stays connected

**Send missing parameter**:
```javascript
ws.send(JSON.stringify({
    cmd: 'DRAW_CIRCLE'
    // Missing x, y, radius
}));
```

**Expected**:
- [ ] Uses defaults: `x=0, y=0, radius=50`
- [ ] Command executes with fallback values

---

## ✅ Verification Checklist: Step 7

- [ ] **WiFi AP**: "RobotWifi" visible in network list
- [ ] **WiFi Connection**: Can connect with password "penbot123"
- [ ] **Static IP**: Assigned 192.168.4.1
- [ ] **HTTP Server**: Responds to requests (200 OK)
- [ ] **WebSocket**: Accepts connections at ws://192.168.4.1:81
- [ ] **Telemetry Stream**: Sends JSON every 100-200ms
- [ ] **Command Parsing**: JSON commands parsed without errors
- [ ] **Command Execution**: DRIVE, STOP, DRAW_CIRCLE commands work
- [ ] **Error Handling**: Invalid commands handled gracefully

---

## 📊 API Reference

### Command Format

All commands sent as JSON over WebSocket:

```json
{
  "cmd": "COMMAND_NAME",
  "param1": value1,
  "param2": value2
}
```

### Supported Commands

| Command | Parameters | Effect |
|---------|-----------|--------|
| `DRIVE` | `linear_mm_s`, `angular_rad_s` | Set motor velocities |
| `STOP` | (none) | Stop all motion |
| `GO_STRAIGHT` | `distance_mm` | Move forward exact distance |
| `ROTATE` | `angle_deg` | Rotate in place |
| `DRAW_CIRCLE` | `x`, `y`, `radius` | Draw circle at (x,y) |
| `DRAW_TRIANGLE` | `x1`, `y1`, `x2`, `y2`, `x3`, `y3` | Draw triangle |
| `DRAW_TEXT` | `text`, `x`, `y`, `size` | Write text |
| `CALIBRATE_IMU` | (none) | Run IMU calibration sequence |
| `GET_TELEMETRY` | (none) | Request single telemetry update |

### Telemetry Format

Sent automatically every 100-200ms:

```json
{
  "pose": {
    "x": 150.2,
    "y": 285.5,
    "heading_deg": 45.3,
    "axle_x": 20.2,
    "axle_y": 155.5
  },
  "ghost_pose": {
    "x": 152.1,
    "y": 287.2,
    "heading_deg": 45.8
  },
  "motors": {
    "left_mm_s": 50.1,
    "right_mm_s": 49.8
  },
  "imu": {
    "heading_deg": 45.3,
    "pitch_deg": -0.5,
    "roll_deg": 0.2
  },
  "battery": {
    "voltage_v": 7.8,
    "percent": 85
  },
  "motion": {
    "mode": "AUTO_PATH",
    "is_moving": true,
    "path_progress_percent": 42.3
  }
}
```

---

## 🎯 Next Steps

**Phase 7 Complete!**
→ Move to [girobot_step8.md](girobot_step8.md): **Web Frontend (HTML/CSS/JS)**

In Phase 8, you will:
1. Create index.html (dashboard layout)
2. Create style.css (responsive styling)
3. Create script.js (WebSocket client + real-time plotting)
4. Test web interface in browser

---

**Estimated Time**: 2 hours  
**Difficulty**: ⭐⭐ Intermediate (mostly verification)  
**Hardware Required**: ESP32, WiFi-enabled computer/phone  

Last Updated: May 11, 2026
