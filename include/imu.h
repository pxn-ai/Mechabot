#ifndef IMU_H
#define IMU_H

#include <MPU9250_WE.h>

// ══════════════════════════════════════════════════════════════
// MPU6500 IMU (MPU9250 clone, gyro + accel only)
// Shares I2C bus with HMC5983 — Wire must be initialized first
// ══════════════════════════════════════════════════════════════

#define MPU_ADDR 0x68

MPU9250_WE imu = MPU9250_WE(MPU_ADDR);
bool imuReady = false;

// Gyro-integrated yaw (raw, will drift — fused in compass.h)
float gyroYaw = 0.0;
unsigned long lastImuMicros = 0;

// Latest readings
float imuPitch = 0.0;
float imuRoll = 0.0;
float imuGyroZ = 0.0;  // °/s

// ──────────────────────────────────────────────
// Initialize MPU6500
// Wire.begin() must be called BEFORE this
// ──────────────────────────────────────────────
bool imuSetup() {
    if (!imu.init()) {
        Serial.println("[IMU] MPU6500 NOT detected at 0x68!");
        return false;
    }

    // Auto-calibrate gyro (keep robot still!)
    Serial.println("[IMU] Calibrating gyro — keep robot STILL...");
    delay(500);
    imu.autoOffsets();
    Serial.println("[IMU] Gyro calibration done");

    // Configure
    imu.setAccRange(MPU9250_ACC_RANGE_4G);
    imu.setGyrRange(MPU9250_GYRO_RANGE_500);
    imu.setGyrDLPF(MPU9250_DLPF_2);   // ~20Hz bandwidth, smooth
    imu.setAccDLPF(MPU9250_DLPF_2);
    imu.setSampleRateDivider(0);  // Max speed

    lastImuMicros = micros();
    imuReady = true;
    Serial.println("[IMU] MPU6500 ready (0x68)");
    return true;
}

// ──────────────────────────────────────────────
// Re-calibrate gyro (call from web endpoint)
// Robot must be stationary
// ──────────────────────────────────────────────
void imuRecalibrate() {
    if (!imuReady) return;
    Serial.println("[IMU] Re-calibrating gyro...");
    imu.autoOffsets();
    gyroYaw = 0.0;
    Serial.println("[IMU] Re-calibration done");
}

// ──────────────────────────────────────────────
// Update readings — call at 50Hz+ in loop()
// ──────────────────────────────────────────────
void imuUpdate() {
    if (!imuReady) return;

    // Time delta
    unsigned long now = micros();
    float dt = (now - lastImuMicros) / 1000000.0;
    lastImuMicros = now;
    if (dt <= 0 || dt > 0.1) return;  // Sanity guard

    // Read gyro
    xyzFloat gyr = imu.getGyrValues();
    imuGyroZ = gyr.z;  // °/s around vertical axis

    // Integrate yaw (will drift, corrected by fusion)
    gyroYaw += imuGyroZ * dt;
    // Wrap to 0-360
    if (gyroYaw >= 360.0) gyroYaw -= 360.0;
    if (gyroYaw < 0.0) gyroYaw += 360.0;

    // Read accelerometer for pitch/roll
    xyzFloat acc = imu.getGValues();
    imuPitch = atan2(acc.x, sqrt(acc.y * acc.y + acc.z * acc.z)) * 180.0 / M_PI;
    imuRoll  = atan2(acc.y, sqrt(acc.x * acc.x + acc.z * acc.z)) * 180.0 / M_PI;
}

// ──────────────────────────────────────────────
// Reset integrated yaw to a known heading
// (called when compass provides a trustworthy reading)
// ──────────────────────────────────────────────
void imuSetYaw(float heading) {
    gyroYaw = heading;
}

#endif // IMU_H
