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
    serialMutex = xSemaphoreCreateMutex();
    return displaySensorQueue != nullptr && alarmSensorQueue != nullptr &&
           serialMutex != nullptr;
}
