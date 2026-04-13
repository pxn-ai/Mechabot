/*
  4WD Mecanum — Vector Kinematics + Sensor Fusion + PID Tuning
  ESP32-S3 | HMC5983 + MPU6500 | 2x L298N
*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "motor_control.h"
#include "compass.h"
#include "imu.h"
#include "web_ui.h"

// ══════════════════════════════════════════════════════════════
// WiFi Access Point
// ══════════════════════════════════════════════════════════════
const char *AP_SSID = "4WD-Car";
const char *AP_PASSWORD = "";

AsyncWebServer server(80);

// ══════════════════════════════════════════════════════════════
// Drive State
// ══════════════════════════════════════════════════════════════
volatile float targetVx = 0.0;
volatile float targetVy = 0.0;
volatile float targetWz = 0.0;

volatile int driveSpeed = 200;
volatile bool compassAssist = true;

float targetHeading = 0.0;
float turnTargetHeading = 0.0;

// ── PID Parameters (tunable from UI) ──
float pidKp = 0.02;   // Proportional — straight-drive correction
float pidKi = 0.0;    // Integral
float pidKd = 0.0;    // Derivative

// Precise turn PID (separate gains)
float turnKp = 3.0;
float turnKi = 0.0;
float turnKd = 0.5;
int   turnMinSpeed = 80;   // Minimum PWM during turns (0-255)
int   turnMaxSpeed = 180;  // Maximum PWM during turns

// PID state
float pidIntegral = 0.0;
float pidLastError = 0.0;
float turnIntegral = 0.0;
float turnLastError = 0.0;

enum Mode { MODE_VECTOR, MODE_PRECISE_TURN, MODE_CALIBRATING };
volatile Mode coreMode = MODE_VECTOR;

// Calibration state
unsigned long calStartTime = 0;
unsigned long calDuration = 8000;

// Timing
unsigned long lastLoopMicros = 0;

// ══════════════════════════════════════════════════════════════
// PID Controller for precise turns (non-blocking)
// ══════════════════════════════════════════════════════════════
bool executePreciseTurn(float dt) {
    float error = getHeadingError(turnTargetHeading);

    // Done?
    if (abs(error) <= 1.5) {
        stopMotors();
        targetHeading = turnTargetHeading;
        turnIntegral = 0.0;
        turnLastError = 0.0;
        Serial.printf("[TURN] Complete! Heading: %.1f\n", getHeading());
        return true;
    }

    // PID calculation
    turnIntegral += error * dt;
    turnIntegral = constrain(turnIntegral, -50.0, 50.0);  // Anti-windup
    float derivative = (dt > 0) ? (error - turnLastError) / dt : 0.0;
    turnLastError = error;

    float output = turnKp * error + turnKi * turnIntegral + turnKd * derivative;

    // Map PID output to motor speed
    float spd = constrain(abs(output) / 180.0, 0.0, 1.0);  // Normalize
    int pwm = map((int)(spd * 1000), 0, 1000, turnMinSpeed, turnMaxSpeed);
    float wzNorm = (float)pwm / 255.0;

    if (error > 0) driveKinematics(0, 0, wzNorm, 255);
    else           driveKinematics(0, 0, -wzNorm, 255);

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
        Serial.printf("[CAL] Done! Offset X=%.2f Y=%.2f (%d samples)\n",
                      calOffsetX, calOffsetY, calResult.samples);
        return true;
    }

    driveKinematics(0, 0, 0.4, 255);

    float mx, my;
    getRawMag(mx, my);

    if (calResult.samples == 0) {
        calResult.minX = calResult.maxX = mx;
        calResult.minY = calResult.maxY = my;
    } else {
        if (mx < calResult.minX) calResult.minX = mx;
        if (mx > calResult.maxX) calResult.maxX = mx;
        if (my < calResult.minY) calResult.minY = my;
        if (my > calResult.maxY) calResult.maxY = my;
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
    Serial.println("\n==========================================");
    Serial.println("  4WD Mecanum — Sensor Fusion + PID Tune");
    Serial.println("==========================================");

    motorSetup();
    compassSetup();
    imuSetup();

    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(500);
    Serial.printf("[WiFi] AP: %s\n", AP_SSID);
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

    // ── Serve UI ──
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
        r->send(200, "text/html", index_html);
    });

    // ── Vector drive ──
    server.on("/vector", HTTP_GET, [](AsyncWebServerRequest *r) {
        if (r->hasParam("vx")) targetVx = r->getParam("vx")->value().toFloat();
        if (r->hasParam("vy")) targetVy = r->getParam("vy")->value().toFloat();
        if (r->hasParam("wz")) targetWz = r->getParam("wz")->value().toFloat();
        if (r->hasParam("speed")) driveSpeed = r->getParam("speed")->value().toInt();
        if (targetWz != 0.0 && compassAssist) targetHeading = getHeading();
        coreMode = MODE_VECTOR;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Stop ──
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *r) {
        targetVx = 0; targetVy = 0; targetWz = 0;
        pidIntegral = 0; pidLastError = 0;
        if (compassAssist) targetHeading = getHeading();
        coreMode = MODE_VECTOR;
        stopMotors();
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Precise turn ──
    server.on("/turn_angle", HTTP_GET, [](AsyncWebServerRequest *r) {
        if (!r->hasParam("angle")) { r->send(400); return; }
        float angle = r->getParam("angle")->value().toFloat();
        turnTargetHeading = normalizeAngle(getHeading() + angle);
        turnIntegral = 0; turnLastError = 0;
        coreMode = MODE_PRECISE_TURN;
        Serial.printf("[CMD] Turn %.1f° → target %.1f°\n", angle, turnTargetHeading);
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Compass toggle ──
    server.on("/compass_toggle", HTTP_GET, [](AsyncWebServerRequest *r) {
        compassAssist = !compassAssist;
        if (compassAssist) targetHeading = getHeading();
        char j[64]; snprintf(j, sizeof(j), "{\"compass\":%s}", compassAssist ? "true" : "false");
        r->send(200, "application/json", j);
    });

    // ── Heading + IMU telemetry ──
    server.on("/heading", HTTP_GET, [](AsyncWebServerRequest *r) {
        char j[256];
        snprintf(j, sizeof(j),
            "{\"heading\":%.1f,\"compass\":%.1f,\"pitch\":%.1f,\"roll\":%.1f,"
            "\"gyroZ\":%.1f,\"compassAssist\":%s,\"calibrated\":%s,"
            "\"imu\":%s,\"mode\":\"%s\"}",
            getHeading(),
            getCompassHeading(),
            imuPitch, imuRoll, imuGyroZ,
            compassAssist ? "true" : "false",
            calibrated ? "true" : "false",
            imuReady ? "true" : "false",
            coreMode == MODE_CALIBRATING ? "calibrating" :
            coreMode == MODE_PRECISE_TURN ? "turning" :
            (targetVx == 0 && targetVy == 0 && targetWz == 0) ? "idle" : "driving"
        );
        r->send(200, "application/json", j);
    });

    // ── Get PID values ──
    server.on("/pid", HTTP_GET, [](AsyncWebServerRequest *r) {
        char j[256];
        snprintf(j, sizeof(j),
            "{\"driveKp\":%.4f,\"driveKi\":%.4f,\"driveKd\":%.4f,"
            "\"turnKp\":%.2f,\"turnKi\":%.2f,\"turnKd\":%.2f,"
            "\"turnMin\":%d,\"turnMax\":%d,\"alpha\":%.2f}",
            pidKp, pidKi, pidKd,
            turnKp, turnKi, turnKd,
            turnMinSpeed, turnMaxSpeed, fusionAlpha);
        r->send(200, "application/json", j);
    });

    // ── Set PID values ──
    server.on("/set_pid", HTTP_GET, [](AsyncWebServerRequest *r) {
        if (r->hasParam("driveKp")) pidKp = r->getParam("driveKp")->value().toFloat();
        if (r->hasParam("driveKi")) pidKi = r->getParam("driveKi")->value().toFloat();
        if (r->hasParam("driveKd")) pidKd = r->getParam("driveKd")->value().toFloat();
        if (r->hasParam("turnKp")) turnKp = r->getParam("turnKp")->value().toFloat();
        if (r->hasParam("turnKi")) turnKi = r->getParam("turnKi")->value().toFloat();
        if (r->hasParam("turnKd")) turnKd = r->getParam("turnKd")->value().toFloat();
        if (r->hasParam("turnMin")) turnMinSpeed = r->getParam("turnMin")->value().toInt();
        if (r->hasParam("turnMax")) turnMaxSpeed = r->getParam("turnMax")->value().toInt();
        if (r->hasParam("alpha")) fusionAlpha = r->getParam("alpha")->value().toFloat();
        Serial.printf("[PID] Drive: P=%.4f I=%.4f D=%.4f\n", pidKp, pidKi, pidKd);
        Serial.printf("[PID] Turn:  P=%.2f I=%.2f D=%.2f min=%d max=%d\n",
                      turnKp, turnKi, turnKd, turnMinSpeed, turnMaxSpeed);
        Serial.printf("[FUSION] Alpha=%.2f\n", fusionAlpha);
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Compass calibration ──
    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *r) {
        calStartTime = millis();
        calResult.samples = 0;
        coreMode = MODE_CALIBRATING;
        r->send(200, "application/json", "{\"status\":\"calibrating\"}");
    });

    // ── Gyro re-calibration ──
    server.on("/imu_calibrate", HTTP_GET, [](AsyncWebServerRequest *r) {
        imuRecalibrate();
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.begin();
    Serial.println("[WEB] Server started -> http://192.168.4.1\n");

    lastLoopMicros = micros();
}

// ══════════════════════════════════════════════════════════════
// Main Loop — 50Hz sensor fusion + motor control
// ══════════════════════════════════════════════════════════════
void loop() {
    // Time delta
    unsigned long now = micros();
    float dt = (now - lastLoopMicros) / 1000000.0;
    lastLoopMicros = now;
    if (dt <= 0 || dt > 0.1) dt = 0.02;  // Sanity

    // Update IMU readings
    imuUpdate();

    // Update fused heading (complementary filter)
    if (imuReady && compassReady) {
        updateFusedHeading(imuGyroZ, dt);
    }

    // Core state machine
    switch (coreMode) {
    case MODE_VECTOR: {
        if (targetVx == 0.0 && targetVy == 0.0 && targetWz == 0.0) {
            stopMotors();
            pidIntegral = 0;
        } else {
            float effectiveWz = targetWz;

            // PID heading lock during translation
            if (compassAssist && targetWz == 0.0) {
                float error = getHeadingError(targetHeading);
                pidIntegral += error * dt;
                pidIntegral = constrain(pidIntegral, -20.0, 20.0);
                float derivative = (dt > 0) ? (error - pidLastError) / dt : 0.0;
                pidLastError = error;

                effectiveWz = pidKp * error + pidKi * pidIntegral + pidKd * derivative;
                effectiveWz = constrain(effectiveWz, -0.5, 0.5);
            }

            driveKinematics(targetVx, targetVy, effectiveWz, driveSpeed);
        }
        break;
    }
    case MODE_PRECISE_TURN:
        if (executePreciseTurn(dt)) coreMode = MODE_VECTOR;
        break;
    case MODE_CALIBRATING:
        if (executeCalibration()) coreMode = MODE_VECTOR;
        break;
    }

    delay(20);
}