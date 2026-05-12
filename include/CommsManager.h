#ifndef COMMSMANAGER_H
#define COMMSMANAGER_H

#include <Arduino.h>
#include "Config.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DriveTrain.h"
#include "MotionController.h"
#include "PoseEstimator.h"
#include "PathPlanner.h"

struct TelemetryPacket {
    float robotX;
    float robotY;
    float robotHeading;
    float ghostX;
    float ghostY;
    float ghostHeading;
    float leftWheelSpeed;
    float rightWheelSpeed;
    long leftWheelSteps;
    long rightWheelSteps;
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;
    float magX;
    float magY;
    float magZ;
    float batteryVoltage;
    bool isMoving;
    int waypointIndex;
    float targetX;
    float targetY;
};

class CommsManager {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    unsigned long lastTelemetryTime = 0;
    
    DriveTrain* driveTrain = nullptr;
    MotionController* motionCtrl = nullptr;
    PoseEstimator* poseEstimator = nullptr;
    PathPlanner* pathPlanner = nullptr;

    // WebSocket event handler
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                               AwsEventType type, void* arg, uint8_t* data, size_t len);

    // Parse incoming command JSON
    void processCommand(const JsonDocument& cmd, AsyncWebSocketClient* client);

public:
    CommsManager();
    ~CommsManager() = default;

    bool begin(DriveTrain* dt, MotionController* mc, PoseEstimator* pe, PathPlanner* pp);
    void update();

    // Send telemetry to all connected clients
    void sendTelemetry(const TelemetryPacket& packet);

    // Broadcast message
    void broadcast(const String& message);

    // Stop server
    void stop();

    // Check if clients are connected
    bool hasClients() const;
};

#endif // COMMSMANAGER_H
