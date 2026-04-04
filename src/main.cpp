/*
    This includes the version with only basic movements (controls)
*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "motor_control.h"
#include "web_ui.h"

// ══════════════════════════════════════════════════════════════
// WiFi Access Point
// ══════════════════════════════════════════════════════════════
const char *AP_SSID = "4WD-Car";
const char *AP_PASSWORD = "";

AsyncWebServer server(80);

// ══════════════════════════════════════════════════════════════
// Setup
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=============================");
  Serial.println("  4WD Car — ESP32-S3");
  Serial.println("=============================");

  motorSetup();
  Serial.println("[HW] Motors ready");

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);
  Serial.printf("[WiFi] AP: %s\n", AP_SSID);
  Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

  // ── Routes ──
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/html", index_html);
  });

  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *r) {
    int speed = r->hasParam("speed")
                    ? constrain(r->getParam("speed")->value().toInt(), 0, 255)
                    : 180;
    moveForward(speed);
    Serial.printf("[CMD] Forward @ %d\n", speed);
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *r) {
    int speed = r->hasParam("speed")
                    ? constrain(r->getParam("speed")->value().toInt(), 0, 255)
                    : 180;
    moveBackward(speed);
    Serial.printf("[CMD] Backward @ %d\n", speed);
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *r) {
    int speed = r->hasParam("speed")
                    ? constrain(r->getParam("speed")->value().toInt(), 0, 255)
                    : 180;
    turnLeft(speed);
    Serial.printf("[CMD] Left @ %d\n", speed);
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *r) {
    int speed = r->hasParam("speed")
                    ? constrain(r->getParam("speed")->value().toInt(), 0, 255)
                    : 180;
    turnRight(speed);
    Serial.printf("[CMD] Right @ %d\n", speed);
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *r) {
    stopMotors();
    Serial.println("[CMD] Stop");
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.begin();
  Serial.println("[WEB] Server started");
  Serial.println("\n>>> Connect to WiFi: 4WD-Car");
  Serial.println(">>> Open: http://192.168.4.1\n");
}

// ══════════════════════════════════════════════════════════════
// Loop
// ══════════════════════════════════════════════════════════════
void loop() {
  // Nothing needed — AsyncWebServer handles requests
}