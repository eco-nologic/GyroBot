#include <Arduino.h>
#include "Config.h"
#include "SelfTest.h"
#include "AutoIMU.h"
#include "DCMotor.h"
#include "VirtualStepper.h"
#include "DriveTrain.h"
#include "Navigation.h"
#include "PoseEstimator.h"
#include "MotionController.h"
#include "PathPlanner.h"
#include "CommsManager.h"
#include "BatteryMonitor.h"
#include "ConfigManager.h"
#include "BluetoothManager.h"
#include <LittleFS.h>

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

ConfigManager config;
BatteryMonitor battery;
Navigation nav;
CommsManager comms;
BluetoothManager ble;

// Motor instances (real or simulated)
IMotor* leftMotor = nullptr;
IMotor* rightMotor = nullptr;

DriveTrain driveTrain(nullptr, nullptr);
PoseEstimator poseEstimator(&driveTrain, &nav);
MotionController motionCtrl(&driveTrain, &poseEstimator);
PathPlanner pathPlanner(&motionCtrl);

// ============================================================================
// TASK HANDLES
// ============================================================================

TaskHandle_t taskControlHandle = nullptr;
TaskHandle_t taskTelemetryHandle = nullptr;

// ============================================================================
// CONTROL TASK (FreeRTOS)
// ============================================================================

void taskControl(void* param) {
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 50ms = 20Hz
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Update all control systems
        poseEstimator.update();
        motionCtrl.update();

        // Debug output every 1 second
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 1000) {
            lastDebugTime = millis();
            Pose pose = poseEstimator.getPose();
            if (EnableMotorDebug) {
                Serial.printf("[Motor] Left: %ld, Right: %ld\n",
                    driveTrain.getLeftEncoderCount(),
                    driveTrain.getRightEncoderCount());
            }
            if (EnableOdometryDebug) {
                Serial.printf("[Pose] X: %.1f, Y: %.1f, θ: %.2f\n",
                    pose.x, pose.y, pose.theta);
            }
        }
    }
}

// ============================================================================
// TELEMETRY TASK (FreeRTOS)
// ============================================================================

void taskTelemetry(void* param) {
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms = 10Hz
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Update sensors
        battery.update();
        nav.update();

        // Build telemetry packet
        Pose robotPose = poseEstimator.getPose();
        Pose ghostPose = poseEstimator.getGhostPose();
        WheelSpeeds wheelSpeeds = driveTrain.getWheelSpeeds();

        TelemetryPacket telem;
        telem.robotX = robotPose.x;
        telem.robotY = robotPose.y;
        telem.robotHeading = robotPose.theta;
        telem.ghostX = ghostPose.x;
        telem.ghostY = ghostPose.y;
        telem.ghostHeading = ghostPose.theta;
        telem.leftWheelSpeed = wheelSpeeds.leftMmS;
        telem.rightWheelSpeed = wheelSpeeds.rightMmS;
        telem.leftWheelSteps = driveTrain.getLeftEncoderCount();
        telem.rightWheelSteps = driveTrain.getRightEncoderCount();
        ImuData raw = nav.getRawData();
        telem.accelX = raw.accelX;
        telem.accelY = raw.accelY;
        telem.accelZ = raw.accelZ;
        telem.gyroX = raw.gyroX;
        telem.gyroY = raw.gyroY;
        telem.gyroZ = raw.gyroZ;
        telem.magX = raw.magX;
        telem.magY = raw.magY;
        telem.magZ = raw.magZ;
        telem.batteryVoltage = battery.getVoltage();
        telem.isMoving = motionCtrl.isMoving();
        telem.waypointIndex = motionCtrl.getCurrentWaypoint();
        
        Waypoint currentTarget = motionCtrl.getCurrentTargetWaypoint();
        telem.targetX = currentTarget.x;
        telem.targetY = currentTarget.y;

        // Calculate bearing to target
        if (telem.isMoving && currentTarget.tolerance > 0) { // Only calculate if there's an active target
            telem.bearingToTarget = atan2(currentTarget.y - robotPose.y, currentTarget.x - robotPose.x);
        } else {
            telem.bearingToTarget = robotPose.theta; // Default to robot's heading if no target
        }

        // Send telemetry
        comms.sendTelemetry(telem);

        // Check battery
        if (battery.isLowBattery()) {
            Serial.println("[Warn] Low battery!");
        }
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.printf("\n\n[Boot] ========================================\n");
    Serial.printf("[Boot] GiRobot Firmware v%s\n", FirmwareVersion);
    Serial.printf("[Boot] System Mode: %s\n", (SYSTEM_MODE == SystemRunMode::Real) ? "REAL" : "SIMULATION");
    Serial.printf("[Boot] ========================================\n\n");

    // Run self-test
    if (!SelfTest::runAllTests()) {
        Serial.println("[ERROR] Self-test failed!");
        while (1) delay(1000);
    }

    // Initialize configuration system
    if (!config.begin()) {
        Serial.println("[ERROR] Config initialization failed!");
        while (1) delay(1000);
    }

    // Mount Filesystem
    if (!LittleFS.begin(true)) {
        Serial.println("[ERROR] LittleFS Mount Failed!");
    } else {
        Serial.println("[Boot] ✅ LittleFS Mounted");
    }

    // Create motor instances based on mode
    if (SYSTEM_MODE == SystemRunMode::Real) {
#ifdef MODE_REAL
        leftMotor = new DCMotor(PinMotorLeftEn, PinMotorLeftIn1, PinMotorLeftIn2,
                                 PinEncoderLeftA, PinEncoderLeftB);
        rightMotor = new DCMotor(PinMotorRightEn, PinMotorRightIn1, PinMotorRightIn2,
                                  PinEncoderRightA, PinEncoderRightB);
        Serial.println("[Boot] Using real hardware motors");
#endif
    } else {
        leftMotor = new VirtualStepper();
        rightMotor = new VirtualStepper();
        Serial.println("[Boot] Using virtual simulated motors");
    }

    // Initialize drivetrain with motors
    driveTrain = DriveTrain(leftMotor, rightMotor);
    driveTrain.begin();
    Serial.println("[Boot] ✅ DriveTrain initialized");

    // Initialize navigation (IMU)
    if (!nav.begin()) {
        Serial.println("[WARN] IMU initialization failed - continuing without IMU");
    } else {
        Serial.println("[Boot] ✅ Navigation (IMU) initialized");
    }

    // Initialize pose estimation
    poseEstimator.begin();
    Serial.println("[Boot] ✅ PoseEstimator initialized");

    // Initialize motion controller
    motionCtrl.begin();
    Serial.println("[Boot] ✅ MotionController initialized");

    // Initialize path planner
    pathPlanner.begin();
    Serial.println("[Boot] ✅ PathPlanner initialized");

    // Initialize battery monitor
    battery.begin();
    Serial.println("[Boot] ✅ BatteryMonitor initialized");

    // Initialize communications
    if (!comms.begin(&driveTrain, &motionCtrl, &poseEstimator, &pathPlanner)) {
        Serial.println("[WARN] CommsManager initialization failed");
    } else {
        Serial.println("[Boot] ✅ CommsManager (WiFi/WebSocket) initialized");
    }

    // Initialize Bluetooth
    if (!ble.begin()) {
        Serial.println("[WARN] BluetoothManager initialization failed");
    } else {
        Serial.println("[Boot] ✅ BluetoothManager initialized");
    }

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        taskControl,      // Function
        "Control",        // Name
        4096,             // Stack size
        nullptr,          // Parameter
        2,                // Priority
        &taskControlHandle,
        1                 // Core 1
    );

    xTaskCreatePinnedToCore(
        taskTelemetry,
        "Telemetry",
        4096,
        nullptr,
        1,
        &taskTelemetryHandle,
        0                 // Core 0
    );

    Serial.println("\n[Boot] ✅ System ready!\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Main loop runs on Core 0
    // Keep it short to not block WiFi stack

    // Update virtual motors if in sim mode
    if (SYSTEM_MODE == SystemRunMode::Sim) {
        static unsigned long lastUpdate = 0;
        unsigned long now = millis();
        float dt = (now - lastUpdate) / 1000.0f;
        lastUpdate = now;

        // Safely cast since we know motor types in sim mode
        if (leftMotor != nullptr) {
            VirtualStepper* vl = static_cast<VirtualStepper*>(leftMotor);
            if (vl) vl->update(dt);
        }
        if (rightMotor != nullptr) {
            VirtualStepper* vr = static_cast<VirtualStepper*>(rightMotor);
            if (vr) vr->update(dt);
        }
    }

    delay(10);
}

// ============================================================================
// EXAMPLE: SIMPLE DRAWING COMMAND (from WebSocket)
// ============================================================================

void exampleDrawCircle() {
    Serial.println("[Example] Drawing circle...");
    pathPlanner.drawCircle(500.0f, 500.0f, 200.0f, 32);
    pathPlanner.executePath();
}

void exampleDrawTriangle() {
    Serial.println("[Example] Drawing triangle...");
    pathPlanner.drawTriangle(500.0f, 500.0f, 300.0f);
    pathPlanner.executePath();
}

void exampleDriveToPoint() {
    Serial.println("[Example] Driving to point (1000, 1000)...");
    motionCtrl.driveToWaypoint(1000.0f, 1000.0f, 50.0f);
}
