#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#include "app_types.h"

extern QueueHandle_t displaySensorQueue;
extern QueueHandle_t alarmSensorQueue;
extern QueueHandle_t displayModeQueue;
extern QueueHandle_t motionStateQueue;
extern SemaphoreHandle_t serialMutex;
extern EventGroupHandle_t systemEvents;

constexpr EventBits_t EVENT_ACTIVE = (1U << 0);
constexpr EventBits_t EVENT_MOTION = (1U << 1);

bool RtosObjects_Create(void);
