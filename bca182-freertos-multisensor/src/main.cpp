#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "display.h"
#include "alarm.h"
#include "input.h"
#include "motion.h"

namespace {
void Stop(const char *message)
{
    Serial_WriteRaw(message);
    __disable_irq();
    for (;;) {}
}
}

int main(void)
{
    Serial_EarlyInit();
    Hardware_Init();
    if (!RtosObjects_Create()) Stop("RTOS object creation failed\r\n");

    if (xTaskCreate(MotionTask, "MotionTask", 256, nullptr, 3, nullptr) != pdPASS)
        Stop("MotionTask creation failed\r\n");
    if (xTaskCreate(InputTask, "InputTask", 256, nullptr, 3, nullptr) != pdPASS)
        Stop("InputTask creation failed\r\n");
    if (xTaskCreate(SensorTask, "SensorTask", 384, nullptr, 2, nullptr) != pdPASS)
        Stop("SensorTask creation failed\r\n");
    if (xTaskCreate(AlarmTask, "AlarmTask", 384, nullptr, 2, nullptr) != pdPASS)
        Stop("AlarmTask creation failed\r\n");
    if (xTaskCreate(DisplayTask, "DisplayTask", 512, nullptr, 1, nullptr) != pdPASS)
        Stop("DisplayTask creation failed\r\n");

    Serial_WriteRaw("Priorities: Motion=3 Input=3 Sensor=2 Alarm=2 Display=1\r\n");
    vTaskStartScheduler();
    Stop("Scheduler failed to start\r\n");
}
