#include "motion_task.h"
#include "motion_logic.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "serial_log.h"

void MotionTask(void *argument)
{
    (void)argument;
    // Hardware_Init already configures PB1 as an input.
    TickType_t lastMotion = xTaskGetTickCount();
    TickType_t lastWake = lastMotion;
    SystemState state = SystemState::ACTIVE;
    bool previousMotion = false;
    Serial_Print("[System] ACTIVE: 15-second inactivity timer started\r\n");
    for (;;) {
        const TickType_t now = xTaskGetTickCount();
        const bool motion = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET;
        if (motion) {
            lastMotion = now;
            xEventGroupSetBits(systemEvents, EVENT_MOTION);
        } else {
            xEventGroupClearBits(systemEvents, EVENT_MOTION);
        }
        if (motion != previousMotion) {
            Serial_Print(motion ? "[MotionTask] Motion detected\r\n" : "[MotionTask] PIR LOW\r\n");
            previousMotion = motion;
        }
        const SystemState next = evaluateSystemState(motion, now, lastMotion,
                                                     pdMS_TO_TICKS(INACTIVITY_TIMEOUT_MS));
        if (next != state) {
            state = next;
            if (state == SystemState::ACTIVE) {
                xEventGroupSetBits(systemEvents, EVENT_ACTIVE);
                Serial_Print("[System] ACTIVE: motion restored operation\r\n");
            } else {
                xEventGroupClearBits(systemEvents, EVENT_ACTIVE);
                Serial_Print("[System] INACTIVE: no motion for 15 seconds\r\n");
            }
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
    }
}
