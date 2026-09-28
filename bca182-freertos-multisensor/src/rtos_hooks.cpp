#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx.h"

#include "serial_log.h"

extern "C" BaseType_t xPortConsumeTickYield(void);

namespace {

void WriteUnsigned(unsigned value)
{
    char digits[11];
    unsigned count = 0;

    if (value == 0U) {
        Serial_WriteRaw("0");
        return;
    }

    while (value > 0U && count < sizeof(digits)) {
        digits[count++] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) {
        const char text[2] = {digits[--count], '\0'};
        Serial_WriteRaw(text);
    }
}

} // namespace

extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char *taskName)
{
    (void)task;
    Serial_WriteRaw("\r\nFREERTOS STACK OVERFLOW: ");
    Serial_WriteRaw(taskName != nullptr ? taskName : "UNKNOWN");
    Serial_WriteRaw("\r\n");
    taskDISABLE_INTERRUPTS();
    while (1) {
    }
}

extern "C" void vApplicationMallocFailedHook(void)
{
    Serial_WriteRaw("\r\nFREERTOS MALLOC FAILED\r\n");
    taskDISABLE_INTERRUPTS();
    while (1) {
    }
}

extern "C" void vAssertCalled(const char *file, int line)
{
    Serial_WriteRaw("\r\nFREERTOS ASSERT: ");
    Serial_WriteRaw(file != nullptr ? file : "UNKNOWN");
    Serial_WriteRaw(":");
    WriteUnsigned(line < 0 ? 0U : static_cast<unsigned>(line));
    Serial_WriteRaw("\r\n");
    taskDISABLE_INTERRUPTS();
    while (1) {
    }
}

extern "C" void vApplicationIdleHook(void)
{
    /*
     * Sleep while nothing is Ready.
     *
     * Important Wokwi optimisation:
     * do NOT call taskYIELD() on every Idle-hook pass.  Some simulator builds
     * return from WFI immediately, and the old code then performed thousands
     * of expensive direct context switches per second.
     *
     * TIM3 sets a pending flag only when xTaskIncrementTick() actually wakes
     * a higher-priority task.  We yield once for that event.
     */
    __DSB();
    __WFI();
    __ISB();

    if (xPortConsumeTickYield() != pdFALSE) {
        taskYIELD();
    }
}
