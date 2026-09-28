#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"

// Tokens enforce A, B, A, B even if execution is delayed.
static SemaphoreHandle_t turnA;
static SemaphoreHandle_t turnB;

static void TaskA(void *argument)
{
    (void)argument;
    for (;;) {
        xSemaphoreTake(turnA, portMAX_DELAY);
        Serial_Print("Task A running\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        xSemaphoreGive(turnB);
    }
}

static void TaskB(void *argument)
{
    (void)argument;
    for (;;) {
        xSemaphoreTake(turnB, portMAX_DELAY);
        Serial_Print("Task B running\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        xSemaphoreGive(turnA);
    }
}

static void Stop(const char *message)
{
    Serial_WriteRaw(message);
    __disable_irq();
    for (;;) {}
}

int main(void)
{
    Serial_EarlyInit();
    Hardware_Init();

    // Keep the reference project's serial mutex and RTOS support objects.
    if (!RtosObjects_Create()) {
        Stop("RTOS object creation failed\r\n");
    }
    turnA = xSemaphoreCreateBinary();
    turnB = xSemaphoreCreateBinary();
    if (turnA == nullptr || turnB == nullptr) {
        Stop("Turn semaphore creation failed\r\n");
    }
    xSemaphoreGive(turnA); // A prints first; B initially waits.

    if (xTaskCreate(TaskA, "TaskA", 256, nullptr, 1, nullptr) != pdPASS ||
        xTaskCreate(TaskB, "TaskB", 256, nullptr, 1, nullptr) != pdPASS) {
        Stop("Task creation failed\r\n");
    }

    Serial_WriteRaw("Starting two diagnostic tasks...\r\n");
    vTaskStartScheduler();
    Stop("Scheduler failed to start\r\n");
}
