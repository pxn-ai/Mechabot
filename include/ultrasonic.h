#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <Arduino.h>

// ──────────────────────────────────────────────
// HC-SR04 Ultrasonic Sensor on ESP32-S3
// ──────────────────────────────────────────────
#define TRIG_PIN  12
#define ECHO_PIN  13

void ultrasonicSetup() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

// Returns distance in cm, or -1 if out of range
float getDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0) return -1.0;

    return (duration * 0.0343) / 2.0;
}

#endif // ULTRASONIC_H
