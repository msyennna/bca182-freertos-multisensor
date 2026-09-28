#pragma once

#include <stdint.h>

struct SensorData {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
    bool dhtValid;
};

enum class DisplayMode : uint8_t {
    TEMPERATURE = 0,
    HUMIDITY,
    LIGHT,
    MOTION
};

enum class AlarmState : uint8_t {
    NORMAL = 0,
    LOW_TEMPERATURE,
    HIGH_TEMPERATURE
};

enum class SystemState : uint8_t {
    ACTIVE = 0,
    INACTIVE
};

constexpr float LOW_TEMPERATURE_LIMIT = 18.0f;
constexpr float HIGH_TEMPERATURE_LIMIT = 30.0f;
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 15000U;
