#include "CommsManager.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

CommsManager::CommsManager() : server(nullptr), ws(nullptr) {}

bool CommsManager::begin() {
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

    ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        handleWebSocketEvent(server, client, type, arg, data, len);
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
            // Parse JSON command
            String msg;
            for (size_t i = 0; i < len; i++) {
                msg += (char)data[i];
            }
            Serial.printf("[Comms] Received: %s\n", msg.c_str());
        }
        break;
    }

    default:
        break;
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
    doc["battery"] = packet.batteryVoltage;
    doc["moving"] = packet.isMoving;
    doc["waypointIndex"] = packet.waypointIndex;
    doc["targetX"] = packet.targetX;
    doc["targetY"] = packet.targetY;

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
