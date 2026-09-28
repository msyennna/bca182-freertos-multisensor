#include "serial_log.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "rtos_objects.h"

void Serial_EarlyInit(void)
{
    /*
     * Bring USART1 up directly from the reset-default 8 MHz HSI clock.
     * This deliberately happens before HAL_Init() so startup diagnostics are
     * still visible even if a later peripheral initialization fails.
     *
     * PA9  = USART1_TX, alternate-function push-pull, 50 MHz
     * PA10 = USART1_RX, floating input
     */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    GPIOA->CRH &= ~((0xFUL << 4U) | (0xFUL << 8U));
    GPIOA->CRH |=  ((0xBUL << 4U) | (0x4UL << 8U));

    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    /* PCLK2 is 8 MHz at reset. BRR=0x45 gives about 115.9 kbaud. */
    USART1->BRR = 0x45U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void UART_SendCharRaw(char c)
{
    uint32_t guard = 0U;

    while ((USART1->SR & USART_SR_TXE) == 0U) {
        if (++guard > 1000000U) {
            return;
        }
    }

    USART1->DR = static_cast<uint8_t>(c);
}

void Serial_WriteRaw(const char *text)
{
    if (text == nullptr) {
        return;
    }

    while (*text != '\0') {
        UART_SendCharRaw(*text++);
    }
}

void Serial_Print(const char *text)
{
    if (serialMutex != nullptr &&
        xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
            Serial_WriteRaw(text);
            xSemaphoreGive(serialMutex);
        }
        return;
    }

    Serial_WriteRaw(text);
}
