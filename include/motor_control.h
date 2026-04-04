#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// ══════════════════════════════════════════════════════════════
// 4WD Mecanum Kinematic Control
// ══════════════════════════════════════════════════════════════
//  L298N #1 (Left Side)                L298N #2 (Right Side)
//  ┌──────────────────┐                ┌──────────────────┐
//  │ Motor A: Front-L │                │ Motor A: Front-R │
//  │ Motor B: Rear-L  │                │ Motor B: Rear-R  │
//  └──────────────────┘                └──────────────────┘

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
#define PWM_RESOLUTION 8 // 0-255

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
// ──────────────────────────────────────────────
void driveMotor(int in1, int in2, int channel, float speed) {
    if (speed > 0.0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW);  }
    else if (speed < 0.0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); }
    else                  { digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  }
    ledcWrite(channel, constrain(abs((int)speed), 0, 255));
}

void setMotorSpeeds(float fl, float fr, float rl, float rr) {
    driveMotor(FL_IN1, FL_IN2, CH_FL, fl);
    driveMotor(FR_IN1, FR_IN2, CH_FR, fr);
    driveMotor(RL_IN1, RL_IN2, CH_RL, rl);
    driveMotor(RR_IN1, RR_IN2, CH_RR, rr);
}

void stopMotors() {
    setMotorSpeeds(0, 0, 0, 0);
}

// ══════════════════════════════════════════════════════════════
// Inverse Kinematics Engine
// ══════════════════════════════════════════════════════════════
// vx: translation lateral (-1.0 to 1.0)
// vy: translation vertical (-1.0 to 1.0)
// wz: rotation around Z  (-1.0 to 1.0)
// speedLimit: absolute hardware limit 0-255
void driveKinematics(float vx, float vy, float wz, int speedLimit) {
    // Kinematic formulas for standard 'X' mecanum layout
    float fl = vy + vx + wz;
    float fr = vy - vx - wz;
    float rl = vy - vx + wz;
    float rr = vy + vx - wz;

    // Normalization to maintain vector ratios if sum exceeds 1.0
    float max_val = max({abs(fl), abs(fr), abs(rl), abs(rr)});
    if (max_val > 1.0) {
        fl /= max_val;
        fr /= max_val;
        rl /= max_val;
        rr /= max_val;
    }

    // Apply absolute max speed mapping
    setMotorSpeeds(fl * speedLimit, fr * speedLimit, rl * speedLimit, rr * speedLimit);
}

#endif // MOTOR_CONTROL_H
