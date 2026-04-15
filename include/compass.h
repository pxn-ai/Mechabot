#ifndef COMPASS_H
#define COMPASS_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <math.h>

// ══════════════════════════════════════════════════════════════
// HMC5983 Compass + Sensor Fusion
// I2C: SDA=GPIO1, SCL=GPIO2
// ══════════════════════════════════════════════════════════════

#define COMPASS_SDA 1
#define COMPASS_SCL 2

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
bool compassReady = false;

// Hard-iron calibration offsets
float calOffsetX = 0.0;
float calOffsetY = 0.0;
bool calibrated = false;

// ── Sensor Fusion ──
float fusedHeading = 0.0;
float fusionAlpha = 0.95;  // Trust gyro 95%, compass 5% per cycle
bool fusionInitialized = false;

// ──────────────────────────────────────────────
// Initialize compass
// ──────────────────────────────────────────────
bool compassSetup() {
    Wire.begin(COMPASS_SDA, COMPASS_SCL);
    Wire.setClock(400000); // 400kHz I2C for faster reads
    if (!mag.begin()) {
        Serial.println("[COMPASS] HMC5983 NOT detected! Check I2C wiring.");
        return false;
    }
    compassReady = true;
    Serial.println("[COMPASS] HMC5983 ready (SDA=1, SCL=2)");
    return true;
}

// ──────────────────────────────────────────────
// Get raw magnetic X, Y (for calibration)
// ──────────────────────────────────────────────
void getRawMag(float &mx, float &my) {
    sensors_event_t event;
    mag.getEvent(&event);
    mx = event.magnetic.x;
    my = event.magnetic.y;
}

// ──────────────────────────────────────────────
// Raw compass heading (0–360°)
// ──────────────────────────────────────────────
float getCompassHeading() {
    if (!compassReady) return -1.0;

    sensors_event_t event;
    mag.getEvent(&event);

    float x = event.magnetic.x - calOffsetX;
    float y = event.magnetic.y - calOffsetY;

    float heading = atan2(y, x);
    if (heading < 0) heading += 2 * PI;

    return heading * 180.0 / M_PI;
}

// ──────────────────────────────────────────────
// Fused heading using complementary filter
// Call this AFTER imuUpdate() each cycle
// gyroYawRate: °/s from gyro Z-axis
// dt: seconds since last call
// ──────────────────────────────────────────────
float updateFusedHeading(float gyroYawRate, float dt) {
    float compassH = getCompassHeading();

    if (!fusionInitialized && compassH >= 0) {
        fusedHeading = compassH;
        fusionInitialized = true;
        return fusedHeading;
    }

    if (compassH < 0) {
        // Compass unavailable — gyro only
        fusedHeading += gyroYawRate * dt;
    } else {
        // Complementary filter
        float gyroEstimate = fusedHeading + gyroYawRate * dt;

        // Handle 360° wrap-around for blending
        float diff = compassH - gyroEstimate;
        if (diff > 180.0) diff -= 360.0;
        else if (diff < -180.0) diff += 360.0;

        fusedHeading = gyroEstimate + (1.0 - fusionAlpha) * diff;
    }

    // Normalize 0–360
    if (fusedHeading >= 360.0) fusedHeading -= 360.0;
    if (fusedHeading < 0.0) fusedHeading += 360.0;

    return fusedHeading;
}

// ──────────────────────────────────────────────
// Get the best available heading
// ──────────────────────────────────────────────
float getHeading() {
    if (fusionInitialized) return fusedHeading;
    return getCompassHeading();
}

// ──────────────────────────────────────────────
// Heading error with 360° wrap-around
// ──────────────────────────────────────────────
float getHeadingError(float target) {
    float current = getHeading();
    float error = target - current;
    if (error > 180.0) error -= 360.0;
    else if (error < -180.0) error += 360.0;
    return error;
}

// ──────────────────────────────────────────────
// Normalize angle to 0–360
// ──────────────────────────────────────────────
float normalizeAngle(float angle) {
    while (angle >= 360.0) angle -= 360.0;
    while (angle < 0.0) angle += 360.0;
    return angle;
}

// ──────────────────────────────────────────────
// Calibration structs
// ──────────────────────────────────────────────
struct CalibrationResult {
    float offsetX, offsetY;
    float minX, maxX, minY, maxY;
    int samples;
};

CalibrationResult runCalibration(unsigned long durationMs) {
    CalibrationResult cal;
    float mx, my;

    getRawMag(mx, my);
    cal.minX = cal.maxX = mx;
    cal.minY = cal.maxY = my;
    cal.samples = 0;

    unsigned long start = millis();
    while (millis() - start < durationMs) {
        getRawMag(mx, my);

        if (mx < cal.minX) cal.minX = mx;
        if (mx > cal.maxX) cal.maxX = mx;
        if (my < cal.minY) cal.minY = my;
        if (my > cal.maxY) cal.maxY = my;

        cal.samples++;
        delay(20);
    }

    cal.offsetX = (cal.maxX + cal.minX) / 2.0;
    cal.offsetY = (cal.maxY + cal.minY) / 2.0;

    calOffsetX = cal.offsetX;
    calOffsetY = cal.offsetY;
    calibrated = true;

    Serial.printf("[COMPASS] Calibration done: %d samples\n", cal.samples);
    Serial.printf("[COMPASS] Offset X=%.2f Y=%.2f\n", calOffsetX, calOffsetY);

    return cal;
}

#endif // COMPASS_H
