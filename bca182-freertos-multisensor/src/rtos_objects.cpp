#include "rtos_objects.h"

QueueHandle_t displaySensorQueue = nullptr;
QueueHandle_t alarmSensorQueue = nullptr;
QueueHandle_t displayModeQueue = nullptr;
QueueHandle_t motionStateQueue = nullptr;
SemaphoreHandle_t serialMutex = nullptr;
EventGroupHandle_t systemEvents = nullptr;

bool RtosObjects_Create(void)
{
    displaySensorQueue = xQueueCreate(1, sizeof(SensorData));
    alarmSensorQueue = xQueueCreate(1, sizeof(SensorData));
    displayModeQueue = xQueueCreate(1, sizeof(DisplayMode));
    motionStateQueue = xQueueCreate(1, sizeof(bool));
    serialMutex = xSemaphoreCreateMutex();
    systemEvents = xEventGroupCreate();

    if (displaySensorQueue == nullptr || alarmSensorQueue == nullptr ||
        displayModeQueue == nullptr || motionStateQueue == nullptr ||
        serialMutex == nullptr || systemEvents == nullptr) {
        return false;
    }

    DisplayMode initialMode = DisplayMode::TEMPERATURE;
    bool noMotion = false;
    xQueueOverwrite(displayModeQueue, &initialMode);
    xQueueOverwrite(motionStateQueue, &noMotion);
    xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
    return true;
}
