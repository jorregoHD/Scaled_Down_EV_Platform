#include <Arduino.h>

#include "Spedometer.h"

namespace {

constexpr uint8_t SPEED_SENSOR_PIN = 13;
constexpr float WHEEL_CIRCUMFERENCE_INCHES = 25.0F;
constexpr uint8_t PULSES_PER_REVOLUTION = 1;
constexpr uint32_t MIN_PULSE_INTERVAL_US = 10000;
constexpr uint32_t ZERO_SPEED_TIMEOUT_US = 3000000;
constexpr float INCHES_PER_MILE = 63360.0F;
constexpr float SECONDS_PER_MINUTE = 60.0F;

volatile uint32_t lastPulseAtUs = 0;
volatile uint32_t pulsePeriodUs = 0;
volatile bool hasPulse = false;

float currentWheelRpm = 0.0F;

void IRAM_ATTR onSpeedPulse() {
    const uint32_t now = micros();

    if (!hasPulse) {
        hasPulse = true;
        lastPulseAtUs = now;
        return;
    }

    const uint32_t period = now - lastPulseAtUs;
    if (period >= MIN_PULSE_INTERVAL_US) {
        pulsePeriodUs = period;
        lastPulseAtUs = now;
    }
}

}  // namespace

namespace Spedometer {

void begin() {
    pinMode(SPEED_SENSOR_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_PIN), onSpeedPulse, FALLING);
}

void update() {
    uint32_t latestPulseAtUs;
    uint32_t latestPulsePeriodUs;
    bool receivedPulse;

    noInterrupts();
    latestPulseAtUs = lastPulseAtUs;
    latestPulsePeriodUs = pulsePeriodUs;
    receivedPulse = hasPulse;
    interrupts();

    if (!receivedPulse || latestPulsePeriodUs == 0 ||
        micros() - latestPulseAtUs > ZERO_SPEED_TIMEOUT_US) {
        currentWheelRpm = 0.0F;
        return;
    }

    currentWheelRpm =
        (60.0F * 1000000.0F) /
        (static_cast<float>(latestPulsePeriodUs) * PULSES_PER_REVOLUTION);
}

float wheelRpm() {
    return currentWheelRpm;
}

float speedMph() {
    return currentWheelRpm * WHEEL_CIRCUMFERENCE_INCHES *
        SECONDS_PER_MINUTE / INCHES_PER_MILE;
}

}  // namespace Spedometer