#ifndef COMMSMANAGER_H
#define COMMSMANAGER_H

#include <Arduino.h>
#include "Config.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

struct TelemetryPacket {
    float robotX;
    float robotY;
    float robotHeading;
    float ghostX;
    float ghostY;
    float ghostHeading;
    float leftWheelSpeed;
    float rightWheelSpeed;
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

    // WebSocket event handler
    static void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                      AwsEventType type, void* arg, uint8_t* data, size_t len);

    // Parse incoming command JSON
    void processCommand(const JsonDocument& cmd, AsyncWebSocketClient* client);

public:
    CommsManager();
    ~CommsManager() = default;

    bool begin();
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
