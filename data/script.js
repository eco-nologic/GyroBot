// ============================================================================
// WebSocket Connection
// ============================================================================

let ws = null;
let robotState = {
    x: 0,
    y: 0,
    theta: 0,
    ghostX: 0,
    ghostY: 0,
    ghostTheta: 0,
    leftSpeed: 0,
    rightSpeed: 0,
    battery: 0,
    moving: false,
    waypointIndex: 0
};

const canvas = document.getElementById('robotCanvas');
const ctx = canvas.getContext('2d');
let animationFrameId = null;

function connect() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const url = `${protocol}//${window.location.host}/ws`;

    console.log('Connecting to', url);

    ws = new WebSocket(url);

    ws.onopen = () => {
        console.log('Connected to robot');
        updateConnectionStatus(true);
        addLog('Connected to GiRobot');
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.type === 'telemetry') {
                updateTelemetry(data);
            }
        } catch (e) {
            console.error('Failed to parse message:', e);
        }
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        addLog('Connection error', 'error');
    };

    ws.onclose = () => {
        console.log('Disconnected from robot');
        updateConnectionStatus(false);
        addLog('Disconnected from GiRobot', 'warning');
        // Attempt reconnect in 5 seconds
        setTimeout(connect, 5000);
    };
}

function sendCommand(cmd) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(cmd));
    }
}

function updateTelemetry(data) {
    robotState.x = data.x || 0;
    robotState.y = data.y || 0;
    robotState.theta = data.heading || 0;
    robotState.ghostX = data.ghostX || 0;
    robotState.ghostY = data.ghostY || 0;
    robotState.ghostTheta = data.ghostHeading || 0;
    robotState.leftSpeed = data.leftSpeed || 0;
    robotState.rightSpeed = data.rightSpeed || 0;
    robotState.battery = data.battery || 0;
    robotState.moving = data.moving || false;
    robotState.waypointIndex = data.waypointIndex || 0;

    updateDisplay();
}

function updateDisplay() {
    // Update position display
    document.getElementById('posX').textContent = robotState.x.toFixed(1);
    document.getElementById('posY').textContent = robotState.y.toFixed(1);
    document.getElementById('posTheta').textContent = robotState.theta.toFixed(2);

    // Update motor speeds
    document.getElementById('leftSpeed').textContent = robotState.leftSpeed.toFixed(1);
    document.getElementById('rightSpeed').textContent = robotState.rightSpeed.toFixed(1);

    // Update battery
    document.getElementById('batteryStatus').textContent = robotState.battery.toFixed(1) + 'V';

    // Redraw canvas
    drawRobotMap();
}

// ============================================================================
// Canvas Drawing
// ============================================================================

function drawRobotMap() {
    const width = canvas.width;
    const height = canvas.height;

    // Clear canvas
    ctx.fillStyle = '#f5f5f5';
    ctx.fillRect(0, 0, width, height);

    // Draw grid
    ctx.strokeStyle = '#ddd';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 10; i++) {
        ctx.beginPath();
        ctx.moveTo((width / 10) * i, 0);
        ctx.lineTo((width / 10) * i, height);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(0, (height / 10) * i);
        ctx.lineTo(width, (height / 10) * i);
        ctx.stroke();
    }

    // Transform coordinates to canvas space
    const scale = 0.2; // 5 pixels per mm, clamped
    const centerX = width / 2;
    const centerY = height / 2;

    const robotX = centerX + robotState.x * scale;
    const robotY = centerY - robotState.y * scale;

    // Draw ghost path (odometry only)
    ctx.strokeStyle = '#cccccc';
    ctx.lineWidth = 2;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(centerX + robotState.ghostX * scale, centerY - robotState.ghostY * scale);
    ctx.stroke();
    ctx.setLineDash([]);

    // Draw robot body
    ctx.fillStyle = '#667eea';
    ctx.beginPath();
    ctx.arc(robotX, robotY, 15, 0, 2 * Math.PI);
    ctx.fill();

    // Draw heading indicator
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(robotX, robotY);
    ctx.lineTo(
        robotX + 20 * Math.cos(robotState.theta),
        robotY - 20 * Math.sin(robotState.theta)
    );
    ctx.stroke();

    // Draw origin
    ctx.fillStyle = '#4caf50';
    ctx.beginPath();
    ctx.arc(centerX, centerY, 5, 0, 2 * Math.PI);
    ctx.fill();
}

// ============================================================================
// UI Event Handlers
// ============================================================================

document.getElementById('btnStop').addEventListener('click', () => {
    sendCommand({ cmd: 'STOP' });
    addLog('STOP command sent');
});

document.getElementById('btnForward').addEventListener('click', () => {
    sendCommand({ cmd: 'FORWARD', speed: 100 });
    addLog('Moving forward');
});

document.getElementById('btnTurnLeft').addEventListener('click', () => {
    sendCommand({ cmd: 'TURN_LEFT', speed: 50 });
    addLog('Turning left');
});

document.getElementById('btnTurnRight').addEventListener('click', () => {
    sendCommand({ cmd: 'TURN_RIGHT', speed: 50 });
    addLog('Turning right');
});

document.getElementById('btnCircle').addEventListener('click', () => {
    sendCommand({ cmd: 'DRAW_CIRCLE', centerX: 500, centerY: 500, radius: 200 });
    addLog('Drawing circle');
});

document.getElementById('btnTriangle').addEventListener('click', () => {
    sendCommand({ cmd: 'DRAW_TRIANGLE', centerX: 500, centerY: 500, size: 300 });
    addLog('Drawing triangle');
});

document.getElementById('btnRectangle').addEventListener('click', () => {
    sendCommand({ cmd: 'DRAW_RECTANGLE', centerX: 500, centerY: 500, width: 400, height: 300 });
    addLog('Drawing rectangle');
});

document.getElementById('btnDrawText').addEventListener('click', () => {
    const text = document.getElementById('textInput').value;
    if (text.trim()) {
        sendCommand({ cmd: 'DRAW_TEXT', text: text, x: 300, y: 300 });
        addLog(`Drawing text: "${text}"`);
    }
});

// ============================================================================
// Joystick Control
// ============================================================================

const joystick = document.getElementById('joystick');
const joystickHandle = document.getElementById('joystickHandle');
let isJoystickActive = false;

joystick.addEventListener('mousedown', () => {
    isJoystickActive = true;
});

document.addEventListener('mouseup', () => {
    isJoystickActive = false;
    joystickHandle.style.left = '50%';
    joystickHandle.style.top = '50%';
    sendCommand({ cmd: 'STOP' });
});

document.addEventListener('mousemove', (e) => {
    if (!isJoystickActive) return;

    const rect = joystick.getBoundingClientRect();
    const centerX = rect.left + rect.width / 2;
    const centerY = rect.top + rect.height / 2;

    let dx = e.clientX - centerX;
    let dy = e.clientY - centerY;

    const distance = Math.sqrt(dx * dx + dy * dy);
    const maxDistance = rect.width / 2;

    if (distance > maxDistance) {
        const ratio = maxDistance / distance;
        dx *= ratio;
        dy *= ratio;
    }

    const handleX = centerX + dx - rect.left;
    const handleY = centerY + dy - rect.top;

    joystickHandle.style.left = handleX + 'px';
    joystickHandle.style.top = handleY + 'px';

    // Send motion command
    const linearVelocity = (dy / maxDistance) * 100;
    const angularVelocity = (dx / maxDistance) * 2;

    sendCommand({
        cmd: 'MOTION',
        linear: linearVelocity,
        angular: angularVelocity
    });
});

// ============================================================================
// Logging & UI Updates
// ============================================================================

function updateConnectionStatus(connected) {
    const status = document.getElementById('connectionStatus');
    if (connected) {
        status.textContent = 'Connected';
        status.className = 'status-value connected';
    } else {
        status.textContent = 'Disconnected';
        status.className = 'status-value disconnected';
    }
}

function addLog(message, level = 'info') {
    const logContainer = document.getElementById('telemetryLog');
    const entry = document.createElement('p');
    entry.className = `log-entry ${level}`;
    const timestamp = new Date().toLocaleTimeString();
    entry.textContent = `[${timestamp}] ${message}`;
    logContainer.appendChild(entry);
    logContainer.scrollTop = logContainer.scrollHeight;

    // Keep only last 50 entries
    while (logContainer.children.length > 50) {
        logContainer.removeChild(logContainer.firstChild);
    }
}

// ============================================================================
// Initialization
// ============================================================================

window.addEventListener('load', () => {
    console.log('Page loaded, connecting to robot...');
    connect();
    drawRobotMap();

    // Update display periodically
    setInterval(drawRobotMap, 100);
});
