#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// ══════════════════════════════════════════════════════════════
// 4WD Motor Control — 2x L298N Motor Drivers on ESP32-S3
// ══════════════════════════════════════════════════════════════
//
//  L298N #1 (Left Side)                L298N #2 (Right Side)
//  ┌──────────────────┐                ┌──────────────────┐
//  │ Motor A: Front-L │                │ Motor A: Front-R │
//  │ Motor B: Rear-L  │                │ Motor B: Rear-R  │
//  └──────────────────┘                └──────────────────┘
//
// ══════════════════════════════════════════════════════════════

// ── L298N #1 — Left Side ──
#define FL_IN1   4    // Front-Left direction
#define FL_IN2   5
#define RL_IN1   7    // Rear-Left direction
#define RL_IN2   6
#define L_ENA    15   // Front-Left PWM (L298N #1 ENA)
#define L_ENB    16   // Rear-Left PWM  (L298N #1 ENB)

// ── L298N #2 — Right Side ──
#define FR_IN1   17   // Front-Right direction
#define FR_IN2   18
#define RR_IN1   8    // Rear-Right direction
#define RR_IN2   9
#define R_ENA    10   // Front-Right PWM (L298N #2 ENA)
#define R_ENB    11   // Rear-Right PWM  (L298N #2 ENB)

// ── ESP32 LEDC PWM config ──
#define PWM_FREQ       1000
#define PWM_RESOLUTION 8      // 0–255

// LEDC Channels
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
// Helper: set all 4 motors
// ──────────────────────────────────────────────
void setMotors(bool fl1, bool fl2,
               bool fr1, bool fr2,
               bool rl1, bool rl2,
               bool rr1, bool rr2,
               int speed) {
    digitalWrite(FL_IN1, fl1); digitalWrite(FL_IN2, fl2);
    digitalWrite(FR_IN1, fr1); digitalWrite(FR_IN2, fr2);
    digitalWrite(RL_IN1, rl1); digitalWrite(RL_IN2, rl2);
    digitalWrite(RR_IN1, rr1); digitalWrite(RR_IN2, rr2);

    ledcWrite(CH_FL, speed);
    ledcWrite(CH_RL, speed);
    ledcWrite(CH_FR, speed);
    ledcWrite(CH_RR, speed);
}

// ──────────────────────────────────────────────
// Movement Functions
// ──────────────────────────────────────────────

void moveForward(int speed) {
    setMotors(HIGH,LOW, HIGH,LOW, HIGH,LOW, HIGH,LOW, speed);
}

void moveBackward(int speed) {
    setMotors(LOW,HIGH, LOW,HIGH, LOW,HIGH, LOW,HIGH, speed);
}

void turnLeft(int speed) {
    setMotors(LOW,HIGH, HIGH,LOW, LOW,HIGH, HIGH,LOW, speed);
}

void turnRight(int speed) {
    setMotors(HIGH,LOW, LOW,HIGH, HIGH,LOW, LOW,HIGH, speed);
}

void stopMotors() {
    setMotors(LOW,LOW, LOW,LOW, LOW,LOW, LOW,LOW, 0);
}

#endif
