#include "CommsManager.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

CommsManager::CommsManager() : server(nullptr), ws(nullptr) {}

bool CommsManager::begin(DriveTrain* dt, MotionController* mc, PoseEstimator* pe, PathPlanner* pp) {
    this->driveTrain = dt;
    this->motionCtrl = mc;
    this->poseEstimator = pe;
    this->pathPlanner = pp;

    Serial.println("[Comms] Starting WiFi AP...");

    // Setup WiFi AP
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(WifiSsid, WifiPassword)) {
        Serial.println("[Comms] WiFi AP failed!");
        return false;
    }

    IPAddress ip;
    ip.fromString(WifiApIp);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    Serial.printf("[Comms] WiFi AP '%s' started at %s\n", WifiSsid, WifiApIp);

    // Setup WebSocket
    server = new AsyncWebServer(WebsocketPort);
    ws = new AsyncWebSocket("/ws");

    ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->handleWebSocketEvent(server, client, type, arg, data, len);
    });

    // Handle favicon requests gracefully to avoid VFS error logs
    server->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });

    // Explicitly handle index routes and check file existence to prevent directory recursion logs
    auto handleIndex = [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            Serial.println("[HTTP] CRITICAL: /index.html not found! Verify 'data' folder upload.");
            request->send(404, "text/plain", "Error 404: index.html missing from filesystem.");
        }
    };

    server->on("/", HTTP_GET, handleIndex);
    server->on("/index.html", HTTP_GET, handleIndex);

    // Serve all other static files from LittleFS (css, js, etc.)
    server->serveStatic("/", LittleFS, "/");

    // Catch-all handler for logging errors (404 Not Found)
    server->onNotFound([](AsyncWebServerRequest *request) {
        Serial.printf("[HTTP] 404 Error - Not Found: %s %s\n", request->methodToString(), request->url().c_str());
        request->send(404, "text/plain", "Not found");
    });

    server->addHandler(ws);
    server->begin();

    Serial.printf("[Comms] WebSocket server started on port %d\n", WebsocketPort);
    return true;
}

void CommsManager::handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                         AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
    case WS_EVT_CONNECT:
        Serial.printf("[Comms] Client %u connected\n", client->id());
        break;

    case WS_EVT_DISCONNECT:
        Serial.printf("[Comms] Client %u disconnected\n", client->id());
        break;

    case WS_EVT_DATA: {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            // Debug: Print the raw JSON to check if keys like "linear_mm_s" match your JS
            if (true) {
                String raw;
                serializeJson(doc, raw);
                Serial.printf("[Comms] JSON Content: %s\n", raw.c_str());
            }
            
            if (error) {
                Serial.printf("[Comms] JSON Parse Error: %s\n", error.c_str());
                return;
            }

            const char* cmd = doc["cmd"] | "";
            Serial.printf("[Comms] Received Command: %s\n", cmd);
            
            processCommand(doc, client);
        }
        break;
    }
    default:
        break;
    }
}

void CommsManager::processCommand(const JsonDocument& doc, AsyncWebSocketClient* client) {
    const char* cmd = doc["cmd"] | "";

    // Helper to stop autonomous motion when manual control starts
    auto stopAuto = [this]() {
        if (motionCtrl) motionCtrl->stop();
    };

    if (strcmp(cmd, "MOTION") == 0) {
        stopAuto();
        
        // Support both direct mm/s and normalized 0-1 joystick inputs
        float linear = doc["linear_mm_s"] | (float)(doc["linear"] | (float)(doc["speed"] | 0.0f)) * MaxLinearSpeedMmS;
        float angular = doc["angular_rad_s"] | (float)(doc["angular"] | (float)(doc["turn"] | 0.0f)) * MaxAngularSpeedRadS;
        
        if (driveTrain) {
            if (abs(linear) > 0.1f || abs(angular) > 0.1f) {
                Serial.printf("[Comms] Driving: Lin=%.1f, Ang=%.2f\n", linear, angular);
            }
            driveTrain->setMotion(linear, angular);
        }
    }
    else if (strcmp(cmd, "FORWARD") == 0) {
        stopAuto();
        if (driveTrain) driveTrain->setMotion(MaxLinearSpeedMmS, 0);
    }
    else if (strcmp(cmd, "BACKWARD") == 0) {
        stopAuto();
        if (driveTrain) driveTrain->setMotion(-MaxLinearSpeedMmS, 0);
    }
    else if (strcmp(cmd, "TURN_LEFT") == 0) {
        stopAuto();
        if (driveTrain) driveTrain->setMotion(0, MaxAngularSpeedRadS); // Full power turn
    }
    else if (strcmp(cmd, "TURN_RIGHT") == 0) {
        stopAuto();
        if (driveTrain) driveTrain->setMotion(0, -MaxAngularSpeedRadS); // Full power turn
    }
    else if (strcmp(cmd, "STOP") == 0) {
        if (motionCtrl) motionCtrl->stop();
        if (driveTrain) driveTrain->stop();
    }
    else if (strcmp(cmd, "DRAW_CIRCLE") == 0) {
        if (pathPlanner) {
            float x = doc["x"] | (poseEstimator ? poseEstimator->getPose().x : 0.0f);
            float y = doc["y"] | (poseEstimator ? poseEstimator->getPose().y : 0.0f);
            float r = doc["radius"] | 100.0f;
            pathPlanner->drawCircle(x, y, r);
            pathPlanner->executePath();
        }
    }
    else if (strcmp(cmd, "DRAW_TRIANGLE") == 0) {
        if (pathPlanner) {
            float x = doc["x"] | (poseEstimator ? poseEstimator->getPose().x : 0.0f);
            float y = doc["y"] | (poseEstimator ? poseEstimator->getPose().y : 0.0f);
            pathPlanner->drawTriangle(x, y, doc["side"] | 100.0f);
            pathPlanner->executePath();
        }
    }
    else if (strcmp(cmd, "DRAW_RECTANGLE") == 0) {
        if (pathPlanner) {
            float x = doc["x"] | (poseEstimator ? poseEstimator->getPose().x : 0.0f);
            float y = doc["y"] | (poseEstimator ? poseEstimator->getPose().y : 0.0f);
            pathPlanner->drawRectangle(x, y, doc["width"] | 100.0f, doc["height"] | 100.0f);
            pathPlanner->executePath();
        }
    }
}

void CommsManager::update() {
    // WebSocket updates handled automatically
}

void CommsManager::sendTelemetry(const TelemetryPacket& packet) {
    if (!ws || !hasClients()) return;

    JsonDocument doc;
    doc["type"] = "telemetry";
    doc["x"] = packet.robotX;
    doc["y"] = packet.robotY;
    doc["heading"] = packet.robotHeading;
    doc["ghostX"] = packet.ghostX;
    doc["ghostY"] = packet.ghostY;
    doc["ghostHeading"] = packet.ghostHeading;
    doc["leftSpeed"] = packet.leftWheelSpeed;
    doc["rightSpeed"] = packet.rightWheelSpeed;
    doc["leftEncoder"] = packet.leftWheelSteps;
    doc["rightEncoder"] = packet.rightWheelSteps;
    doc["ax"] = packet.accelX;
    doc["ay"] = packet.accelY;
    doc["az"] = packet.accelZ;
    doc["gx"] = packet.gyroX;
    doc["gy"] = packet.gyroY;
    doc["gz"] = packet.gyroZ;
    doc["mx"] = packet.magX;
    doc["my"] = packet.magY;
    doc["mz"] = packet.magZ;
    doc["battery"] = packet.batteryVoltage;
    doc["moving"] = packet.isMoving;
    doc["waypointIndex"] = packet.waypointIndex;
    doc["targetX"] = packet.targetX;
    doc["targetY"] = packet.targetY;
    doc["bearing"] = packet.bearingToTarget;

    String json;
    serializeJson(doc, json);

    ws->textAll(json);
}

void CommsManager::broadcast(const String& message) {
    if (ws) {
        ws->textAll(message);
    }
}

void CommsManager::stop() {
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    if (ws) {
        delete ws;
        ws = nullptr;
    }
}

bool CommsManager::hasClients() const {
    return ws && ws->count() > 0;
}
