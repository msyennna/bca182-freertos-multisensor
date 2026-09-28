#include "encoder.h"
#include "navigation.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "serial_log.h"
#include <cstdio>

namespace {
// ISR-to-task ring. Task reads under a short interrupt lock; ISR never calls RTOS.
constexpr unsigned CAPACITY = 32;
volatile int8_t turns[CAPACITY];
volatile unsigned head = 0, tail = 0;
volatile unsigned overflow = 0;

bool PopTurn(int8_t &turn)
{
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    const bool available = tail != head;
    if (available) {
        turn = turns[tail];
        tail = (tail + 1U) % CAPACITY;
    }
    __set_PRIMASK(mask);
    return available;
}

void EncoderInit()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(GPIOB, &gpio);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
    // Priority 4 remains responsive during the DHT BASEPRI=5 critical section.
    // This ISR must NOT call FreeRTOS APIs or request a context switch.
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
}

extern "C" void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
        const int8_t turn = (GPIOB->IDR & GPIO_PIN_13) ? 1 : -1;
        const unsigned next = (head + 1U) % CAPACITY;
        if (next != tail) {
            turns[head] = turn;
            __DMB();
            head = next;
        } else {
            ++overflow;
        }
    }
}

void InputTask(void *argument)
{
    (void)argument;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    EncoderInit();
    Serial_Print("[InputTask] Encoder ready: CLK PB12, DT PB13\r\n");
    unsigned reportedOverflow = 0;
    for (;;) {
        int8_t turn;
        while (PopTurn(turn)) {
            mode = Navigate(mode, turn > 0);
            xQueueOverwrite(displayModeQueue, &mode);
            char line[80];
            std::snprintf(line, sizeof(line), "[InputTask] %s -> %s\r\n",
                          turn > 0 ? "CW" : "CCW", ModeName(mode));
            Serial_Print(line);
        }
        if (overflow != reportedOverflow) {
            reportedOverflow = overflow;
            Serial_Print("[InputTask] Encoder buffer full; some turns lost\r\n");
        }
        // One 50 ms tick in the inherited 20 Hz scheduler.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
