/*
  This version include with two joystick control
  - Translational
  - Rotational
*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "compass.h"
#include "motor_control.h"
#include "web_ui.h"

// ══════════════════════════════════════════════════════════════
// WiFi Access Point
// ══════════════════════════════════════════════════════════════
const char *AP_SSID = "4WD-Car";
const char *AP_PASSWORD = "";

AsyncWebServer server(80);

// ══════════════════════════════════════════════════════════════
// Drive State Machine
// ══════════════════════════════════════════════════════════════
volatile float targetVx = 0.0;
volatile float targetVy = 0.0;
volatile float targetWz = 0.0;

volatile int driveSpeed = 200;
volatile bool compassAssist = true;

float targetHeading = 0.0;
float turnTargetHeading = 0.0;
float Kp = 0.02; // Normalized P-gain for Wz (output scale is -1.0 to 1.0)
int turnSpeed = 150;

enum Mode { MODE_VECTOR, MODE_PRECISE_TURN, MODE_CALIBRATING };
volatile Mode coreMode = MODE_VECTOR;

// Calibration state
volatile bool calibrating = false;
unsigned long calStartTime = 0;
unsigned long calDuration = 8000;

// ══════════════════════════════════════════════════════════════
// Precise turn logic (non-blocking)
// ══════════════════════════════════════════════════════════════
bool executePreciseTurn() {
  if (!compassReady) {
    stopMotors();
    return true;
  }

  float error = getHeadingError(turnTargetHeading);

  if (abs(error) <= 2.0) {
    stopMotors();
    targetHeading = turnTargetHeading;
    Serial.printf("[TURN] Complete! Heading: %.1f\n", getHeading());
    return true;
  }

  float spd =
      (abs(error) > 20) ? 0.6 : map((int)abs(error), 2, 20, 30, 60) / 100.0;

  // Wz only
  if (error > 0)
    driveKinematics(0, 0, spd, driveSpeed);
  else
    driveKinematics(0, 0, -spd, driveSpeed);

  return false;
}

// ══════════════════════════════════════════════════════════════
// Calibration logic
// ══════════════════════════════════════════════════════════════
CalibrationResult calResult;
bool executeCalibration() {
  if (millis() - calStartTime >= calDuration) {
    stopMotors();
    calOffsetX = calResult.offsetX;
    calOffsetY = calResult.offsetY;
    calibrated = true;
    Serial.printf("[CAL] Done! Offset X=%.2f Y=%.2f (%d samples)\n", calOffsetX,
                  calOffsetY, calResult.samples);
    return true;
  }

  // Spin slowly
  driveKinematics(0, 0, 0.4, 255);

  float mx, my;
  getRawMag(mx, my);

  if (calResult.samples == 0) {
    calResult.minX = calResult.maxX = mx;
    calResult.minY = calResult.maxY = my;
  } else {
    if (mx < calResult.minX)
      calResult.minX = mx;
    if (mx > calResult.maxX)
      calResult.maxX = mx;
    if (my < calResult.minY)
      calResult.minY = my;
    if (my > calResult.maxY)
      calResult.maxY = my;
  }
  calResult.samples++;
  calResult.offsetX = (calResult.maxX + calResult.minX) / 2.0;
  calResult.offsetY = (calResult.maxY + calResult.minY) / 2.0;

  return false;
}

// ══════════════════════════════════════════════════════════════
// Setup
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=================================");
  Serial.println("  4WD Mecanum Vector Kinematics");
  Serial.println("=================================");

  motorSetup();
  compassSetup();

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);
  Serial.printf("[WiFi] AP: %s\n", AP_SSID);
  Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/html", index_html);
  });

  // ── Unified Vector Command ──
  server.on("/vector", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (r->hasParam("vx"))
      targetVx = r->getParam("vx")->value().toFloat();
    if (r->hasParam("vy"))
      targetVy = r->getParam("vy")->value().toFloat();
    if (r->hasParam("wz"))
      targetWz = r->getParam("wz")->value().toFloat();
    if (r->hasParam("speed"))
      driveSpeed = r->getParam("speed")->value().toInt();

    // If manual rotation is applied, reset compass target to current heading
    if (targetWz != 0.0 && compassReady) {
      targetHeading = getHeading();
    }

    coreMode = MODE_VECTOR;
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ── Stop ──
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *r) {
    targetVx = 0.0;
    targetVy = 0.0;
    targetWz = 0.0;
    if (compassReady)
      targetHeading = getHeading();
    coreMode = MODE_VECTOR;
    stopMotors();
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ── Precise turn ──
  server.on("/turn_angle", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (!r->hasParam("angle")) {
      r->send(400);
      return;
    }
    float angle = r->getParam("angle")->value().toFloat();
    turnTargetHeading = normalizeAngle(getHeading() + angle);
    coreMode = MODE_PRECISE_TURN;
    Serial.printf("[CMD] Precise Turn %.1f°\n", angle);
    r->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ── Compass toggle ──
  server.on("/compass_toggle", HTTP_GET, [](AsyncWebServerRequest *r) {
    compassAssist = !compassAssist;
    if (compassAssist && compassReady)
      targetHeading = getHeading();
    char j[64];
    snprintf(j, sizeof(j), "{\"compass\":%s}",
             compassAssist ? "true" : "false");
    r->send(200, "application/json", j);
  });

  // ── Utility ──
  server.on("/heading", HTTP_GET, [](AsyncWebServerRequest *r) {
    char j[128];
    snprintf(
        j, sizeof(j),
        "{\"heading\":%.1f,\"compass\":%s,\"calibrated\":%s,\"mode\":\"%s\"}",
        getHeading(), compassAssist ? "true" : "false",
        calibrated ? "true" : "false",
        coreMode == MODE_CALIBRATING                        ? "calibrating"
        : coreMode == MODE_PRECISE_TURN                     ? "turning"
        : (targetVx == 0 && targetVy == 0 && targetWz == 0) ? "idle"
                                                            : "driving");
    r->send(200, "application/json", j);
  });

  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *r) {
    calStartTime = millis();
    calResult.samples = 0;
    coreMode = MODE_CALIBRATING;
    r->send(200, "application/json", "{\"status\":\"calibrating\"}");
  });

  server.begin();
  Serial.println("[WEB] Server started -> http://192.168.4.1\n");
}

// ══════════════════════════════════════════════════════════════
// Main Loop
// ══════════════════════════════════════════════════════════════
void loop() {
  switch (coreMode) {
  case MODE_VECTOR: {
    if (targetVx == 0.0 && targetVy == 0.0 && targetWz == 0.0) {
      stopMotors();
    } else {
      float effectiveWz = targetWz;

      // Overlay Magnetic P-Controller if translating without manual spinning
      if (compassAssist && compassReady && targetWz == 0.0) {
        float error = getHeadingError(targetHeading);
        effectiveWz = constrain(Kp * error, -0.5, 0.5);
      }

      driveKinematics(targetVx, targetVy, effectiveWz, driveSpeed);
    }
    break;
  }
  case MODE_PRECISE_TURN:
    if (executePreciseTurn())
      coreMode = MODE_VECTOR;
    break;
  case MODE_CALIBRATING:
    if (executeCalibration())
      coreMode = MODE_VECTOR;
    break;
  }
  delay(20); // 50hz
}