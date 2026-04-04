#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "motor_control.h"
#include "compass.h"
#include "web_ui.h"

// ══════════════════════════════════════════════════════════════
// WiFi Access Point
// ══════════════════════════════════════════════════════════════
const char* AP_SSID     = "4WD-Car";
const char* AP_PASSWORD = "";

AsyncWebServer server(80);

// ══════════════════════════════════════════════════════════════
// Drive State Machine
// ══════════════════════════════════════════════════════════════
enum DriveMode {
    MODE_IDLE,
    MODE_FORWARD,
    MODE_BACKWARD,
    MODE_TURN_LEFT,
    MODE_TURN_RIGHT,
    MODE_STRAFE_LEFT,
    MODE_STRAFE_RIGHT,
    MODE_DIAG_FL,
    MODE_DIAG_FR,
    MODE_DIAG_BL,
    MODE_DIAG_BR,
    MODE_PRECISE_TURN,
    MODE_CALIBRATING
};

volatile DriveMode driveMode = MODE_IDLE;
volatile int driveSpeed = 180;
volatile bool compassAssist = true;   // Compass-assisted by default
float targetHeading = 0.0;
float turnTargetHeading = 0.0;
float Kp = 2.5;                       // Proportional gain for compass correction
int turnSpeed = 150;                   // Speed during precise turns

// Calibration state
volatile bool calibrating = false;
unsigned long calStartTime = 0;
unsigned long calDuration = 8000;      // 8 seconds of spinning for cal

// ══════════════════════════════════════════════════════════════
// Compass-assisted forward/backward drive
// ══════════════════════════════════════════════════════════════
void compassDriveStraight(int baseSpeed, bool forward) {
    if (!compassReady || !compassAssist) {
        // Fallback: no compass
        if (forward) moveForward(baseSpeed);
        else moveBackward(baseSpeed);
        return;
    }

    float error = getHeadingError(targetHeading);
    float correction = Kp * error;

    int leftSpeed  = baseSpeed + (int)correction;
    int rightSpeed = baseSpeed - (int)correction;

    leftSpeed  = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    if (forward) {
        setMotorSpeeds(leftSpeed, rightSpeed, leftSpeed, rightSpeed);
    } else {
        setMotorSpeeds(-leftSpeed, -rightSpeed, -leftSpeed, -rightSpeed);
    }
}

// ══════════════════════════════════════════════════════════════
// Precise turn logic (non-blocking, runs in loop)
// Returns true when turn is complete
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

    // Variable speed: slow down as we approach target
    int spd = (abs(error) > 20) ? turnSpeed : map((int)abs(error), 2, 20, 100, turnSpeed);

    if (error > 0) {
        turnRight(spd);
    } else {
        turnLeft(spd);
    }
    return false;
}

// ══════════════════════════════════════════════════════════════
// Calibration logic (non-blocking, runs in loop)
// ══════════════════════════════════════════════════════════════
CalibrationResult calResult;
bool executeCalibration() {
    if (millis() - calStartTime >= calDuration) {
        stopMotors();
        // Compute final offsets from collected data
        calOffsetX = calResult.offsetX;
        calOffsetY = calResult.offsetY;
        calibrated = true;
        Serial.printf("[CAL] Done! Offset X=%.2f Y=%.2f (%d samples)\n",
                      calOffsetX, calOffsetY, calResult.samples);
        return true;
    }

    // Spin slowly for calibration
    turnRight(120);

    // Sample magnetic field
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

    // Update offsets live
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
    Serial.println("  4WD Mecanum Car — ESP32-S3");
    Serial.println("  Compass + Mecanum Edition");
    Serial.println("=================================");

    motorSetup();
    Serial.println("[HW] Motors ready");

    compassSetup();

    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(500);
    Serial.printf("[WiFi] AP: %s\n", AP_SSID);
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

    // ── Serve UI ──
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
        r->send(200, "text/html", index_html);
    });

    // ── Basic movements ──
    server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        if (compassAssist && compassReady) targetHeading = getHeading();
        driveMode = MODE_FORWARD;
        Serial.printf("[CMD] Forward @ %d (compass=%s)\n", driveSpeed, compassAssist ? "ON" : "OFF");
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        if (compassAssist && compassReady) targetHeading = getHeading();
        driveMode = MODE_BACKWARD;
        Serial.printf("[CMD] Backward @ %d\n", driveSpeed);
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/left", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_TURN_LEFT;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/right", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_TURN_RIGHT;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveMode = MODE_IDLE;
        stopMotors();
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Mecanum strafes ──
    server.on("/strafe_left", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_STRAFE_LEFT;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/strafe_right", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_STRAFE_RIGHT;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/diag_fl", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_DIAG_FL;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/diag_fr", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_DIAG_FR;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/diag_bl", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_DIAG_BL;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/diag_br", HTTP_GET, [](AsyncWebServerRequest *r) {
        driveSpeed = r->hasParam("speed") ? constrain(r->getParam("speed")->value().toInt(), 0, 255) : 180;
        driveMode = MODE_DIAG_BR;
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Precise turn ──
    server.on("/turn_angle", HTTP_GET, [](AsyncWebServerRequest *r) {
        if (!r->hasParam("angle")) {
            r->send(400, "application/json", "{\"error\":\"missing angle\"}");
            return;
        }
        float angle = r->getParam("angle")->value().toFloat();
        float current = getHeading();
        turnTargetHeading = normalizeAngle(current + angle);
        driveMode = MODE_PRECISE_TURN;
        Serial.printf("[CMD] Turn %.1f° → target %.1f°\n", angle, turnTargetHeading);
        r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // ── Compass toggle ──
    server.on("/compass_toggle", HTTP_GET, [](AsyncWebServerRequest *r) {
        compassAssist = !compassAssist;
        char json[64];
        snprintf(json, sizeof(json), "{\"compass\":%s}", compassAssist ? "true" : "false");
        Serial.printf("[CMD] Compass assist: %s\n", compassAssist ? "ON" : "OFF");
        r->send(200, "application/json", json);
    });

    // ── Heading data ──
    server.on("/heading", HTTP_GET, [](AsyncWebServerRequest *r) {
        char json[128];
        float h = getHeading();
        snprintf(json, sizeof(json),
            "{\"heading\":%.1f,\"compass\":%s,\"calibrated\":%s,\"mode\":\"%s\"}",
            h,
            compassAssist ? "true" : "false",
            calibrated ? "true" : "false",
            driveMode == MODE_CALIBRATING ? "calibrating" :
            driveMode == MODE_PRECISE_TURN ? "turning" :
            driveMode == MODE_IDLE ? "idle" : "driving"
        );
        r->send(200, "application/json", json);
    });

    // ── Calibration ──
    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *r) {
        calStartTime = millis();
        calResult.samples = 0;
        driveMode = MODE_CALIBRATING;
        Serial.println("[CMD] Calibration started (8s spin)");
        r->send(200, "application/json", "{\"status\":\"calibrating\"}");
    });

    server.begin();
    Serial.println("[WEB] Server started on port 80");
    Serial.println("\n>>> Connect to WiFi: 4WD-Car");
    Serial.println(">>> Open: http://192.168.4.1\n");
}

// ══════════════════════════════════════════════════════════════
// Main Loop — runs at ~50Hz for compass correction
// ══════════════════════════════════════════════════════════════
void loop() {
    switch (driveMode) {
        case MODE_FORWARD:
            compassDriveStraight(driveSpeed, true);
            break;
        case MODE_BACKWARD:
            compassDriveStraight(driveSpeed, false);
            break;
        case MODE_TURN_LEFT:
            turnLeft(driveSpeed);
            break;
        case MODE_TURN_RIGHT:
            turnRight(driveSpeed);
            break;
        case MODE_STRAFE_LEFT:
            strafeLeft(driveSpeed);
            break;
        case MODE_STRAFE_RIGHT:
            strafeRight(driveSpeed);
            break;
        case MODE_DIAG_FL:
            diagFL(driveSpeed);
            break;
        case MODE_DIAG_FR:
            diagFR(driveSpeed);
            break;
        case MODE_DIAG_BL:
            diagBL(driveSpeed);
            break;
        case MODE_DIAG_BR:
            diagBR(driveSpeed);
            break;
        case MODE_PRECISE_TURN:
            if (executePreciseTurn()) {
                driveMode = MODE_IDLE;
            }
            break;
        case MODE_CALIBRATING:
            if (executeCalibration()) {
                driveMode = MODE_IDLE;
            }
            break;
        case MODE_IDLE:
        default:
            break;
    }

    delay(20); // 50Hz loop
}