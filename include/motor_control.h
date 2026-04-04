#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// ══════════════════════════════════════════════════════════════
// 4WD Mecanum Motor Control — 2x L298N on ESP32-S3
// ══════════════════════════════════════════════════════════════
//
//  L298N #1 (Left Side)                L298N #2 (Right Side)
//  ┌──────────────────┐                ┌──────────────────┐
//  │ Motor A: Front-L │                │ Motor A: Front-R │
//  │ Motor B: Rear-L  │                │ Motor B: Rear-R  │
//  └──────────────────┘                └──────────────────┘
//
//  Mecanum Wheel Layout (top view, arrows show roller axis):
//    FL ╲  ╱ FR
//    RL ╱  ╲ RR
//
// ══════════════════════════════════════════════════════════════

// ── L298N #1 — Left Side ──
#define FL_IN1   4
#define FL_IN2   5
#define RL_IN1   7
#define RL_IN2   6
#define L_ENA    15   // Front-Left PWM
#define L_ENB    16   // Rear-Left PWM

// ── L298N #2 — Right Side ──
#define FR_IN1   17
#define FR_IN2   18
#define RR_IN1   8
#define RR_IN2   9
#define R_ENA    10   // Front-Right PWM
#define R_ENB    11   // Rear-Right PWM

// ── LEDC PWM ──
#define PWM_FREQ       1000
#define PWM_RESOLUTION 8

#define CH_FL  0
#define CH_RL  1
#define CH_FR  2
#define CH_RR  3

// ──────────────────────────────────────────────
// Setup
// ──────────────────────────────────────────────
void motorSetup() {
    int dirPins[] = {FL_IN1, FL_IN2, RL_IN1, RL_IN2,
                     FR_IN1, FR_IN2, RR_IN1, RR_IN2};
    for (int pin : dirPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    ledcSetup(CH_FL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(CH_RL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(CH_FR, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(CH_RR, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(L_ENA, CH_FL);
    ledcAttachPin(L_ENB, CH_RL);
    ledcAttachPin(R_ENA, CH_FR);
    ledcAttachPin(R_ENB, CH_RR);

    ledcWrite(CH_FL, 0);
    ledcWrite(CH_RL, 0);
    ledcWrite(CH_FR, 0);
    ledcWrite(CH_RR, 0);
}

// ──────────────────────────────────────────────
// Low-level: drive a single motor
//   dir: 1=forward, -1=backward, 0=stop
//   speed: 0–255
// ──────────────────────────────────────────────
void driveMotor(int in1, int in2, int channel, int dir, int speed) {
    if (dir > 0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW);  }
    else if (dir < 0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); }
    else              { digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  }
    ledcWrite(channel, abs(speed));
}

// ──────────────────────────────────────────────
// Per-motor speed control (for compass correction)
// Positive = forward, negative = backward
// ──────────────────────────────────────────────
void setMotorSpeeds(int fl, int fr, int rl, int rr) {
    driveMotor(FL_IN1, FL_IN2, CH_FL, (fl > 0) ? 1 : (fl < 0) ? -1 : 0, abs(fl));
    driveMotor(FR_IN1, FR_IN2, CH_FR, (fr > 0) ? 1 : (fr < 0) ? -1 : 0, abs(fr));
    driveMotor(RL_IN1, RL_IN2, CH_RL, (rl > 0) ? 1 : (rl < 0) ? -1 : 0, abs(rl));
    driveMotor(RR_IN1, RR_IN2, CH_RR, (rr > 0) ? 1 : (rr < 0) ? -1 : 0, abs(rr));
}

// ══════════════════════════════════════════════════════════════
// High-level movement (uniform speed)
// ══════════════════════════════════════════════════════════════

void stopMotors() {
    setMotorSpeeds(0, 0, 0, 0);
}

void moveForward(int s) {
    setMotorSpeeds(s, s, s, s);
}

void moveBackward(int s) {
    setMotorSpeeds(-s, -s, -s, -s);
}

// Spin in place
void turnLeft(int s) {
    setMotorSpeeds(-s, s, -s, s);
}

void turnRight(int s) {
    setMotorSpeeds(s, -s, s, -s);
}

// ══════════════════════════════════════════════════════════════
// Mecanum-specific movements
// ══════════════════════════════════════════════════════════════

// Strafe (sideways, no rotation)
void strafeLeft(int s) {
    // FL backward, FR forward, RL forward, RR backward
    setMotorSpeeds(-s, s, s, -s);
}

void strafeRight(int s) {
    // FL forward, FR backward, RL backward, RR forward
    setMotorSpeeds(s, -s, -s, s);
}

// Diagonals
void diagFL(int s) {
    // Front-Left: FL stop, FR fwd, RL fwd, RR stop
    setMotorSpeeds(0, s, s, 0);
}

void diagFR(int s) {
    // Front-Right: FL fwd, FR stop, RL stop, RR fwd
    setMotorSpeeds(s, 0, 0, s);
}

void diagBL(int s) {
    // Back-Left: FL stop, FR bwd, RL bwd, RR stop
    setMotorSpeeds(0, -s, -s, 0);
}

void diagBR(int s) {
    // Back-Right: FL bwd, FR stop, RL stop, RR bwd
    setMotorSpeeds(-s, 0, 0, -s);
}

#endif // MOTOR_CONTROL_H
