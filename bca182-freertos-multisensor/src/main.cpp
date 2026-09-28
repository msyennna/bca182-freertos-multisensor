#include <cstdio>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"

namespace {

GPIO_TypeDef *const DHT_PORT = GPIOB;
constexpr uint16_t DHT_PIN = GPIO_PIN_0;

void DHT_SetOutput(void)
{
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = DHT_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT_PORT, &gpio);
}

void DHT_SetInput(void)
{
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = DHT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT_PORT, &gpio);
}

inline GPIO_PinState DHT_ReadPinFast()
{
    return ((DHT_PORT->IDR & DHT_PIN) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

bool DelayUs(uint16_t microseconds)
{
    /* TIM4 runs at 1 MHz: one counter count equals one microsecond. */
    const uint16_t start = static_cast<uint16_t>(TIM4->CNT);
    uint32_t guard = 0U;
    const uint32_t guardLimit = static_cast<uint32_t>(microseconds) * 80U + 2000U;

    while (static_cast<uint16_t>(TIM4->CNT - start) < microseconds) {
        if (++guard >= guardLimit) {
            return false;
        }
        __NOP();
    }
    return true;
}

bool WaitWhile(GPIO_PinState state, uint16_t timeoutUs, uint16_t *elapsedUs = nullptr)
{
    const uint16_t start = static_cast<uint16_t>(TIM4->CNT);
    uint32_t guard = 0U;
    const uint32_t guardLimit = static_cast<uint32_t>(timeoutUs) * 120U + 3000U;

    while (DHT_ReadPinFast() == state) {
        const uint16_t elapsed = static_cast<uint16_t>(TIM4->CNT - start);
        if (elapsed >= timeoutUs || ++guard >= guardLimit) {
            if (elapsedUs != nullptr) {
                *elapsedUs = elapsed;
            }
            return false;
        }
    }

    if (elapsedUs != nullptr) {
        *elapsedUs = static_cast<uint16_t>(TIM4->CNT - start);
    }
    return true;
}

bool ReadDHT22(float &temperature, float &humidity)
{
    uint8_t bytes[5] = {0, 0, 0, 0, 0};
    bool ok = true;

    DHT_SetOutput();
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_RESET);
    if (!DelayUs(1200)) {
        HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
        DHT_SetInput();
        return false;
    }
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
    if (!DelayUs(30)) { DHT_SetInput(); return false; }
    DHT_SetInput();

    if (!WaitWhile(GPIO_PIN_SET, 120)) ok = false;
    if (ok && !WaitWhile(GPIO_PIN_RESET, 120)) ok = false;
    if (ok && !WaitWhile(GPIO_PIN_SET, 120)) ok = false;

    for (int bit = 0; bit < 40 && ok; ++bit) {
        if (!WaitWhile(GPIO_PIN_RESET, 100)) {
            ok = false;
            break;
        }

        uint16_t highTime = 0U;
        if (!WaitWhile(GPIO_PIN_SET, 120, &highTime)) {
            ok = false;
            break;
        }

        bytes[bit / 8] <<= 1;
        if (highTime > 40U) {
            bytes[bit / 8] |= 1U;
        }
    }


    if (!ok) {
        return false;
    }

    const uint8_t checksum = static_cast<uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if (checksum != bytes[4]) {
        return false;
    }

    const uint16_t humidityRaw = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
    const uint16_t magnitude = static_cast<uint16_t>(((bytes[2] & 0x7FU) << 8) | bytes[3]);

    humidity = static_cast<float>(humidityRaw) / 10.0f;
    temperature = static_cast<float>(magnitude) / 10.0f;
    if ((bytes[2] & 0x80U) != 0U) {
        temperature = -temperature;
    }

    return true;
}

// Integer formatting avoids requiring printf floating-point support.
void PrintReading(const char *label, float value, const char *unit)
{
    int tenths = static_cast<int>(value * 10.0f + (value >= 0 ? 0.5f : -0.5f));
    const bool negative = tenths < 0;
    if (negative) tenths = -tenths;
    char line[64];
    std::snprintf(line, sizeof(line), "%s: %s%d.%02d %s\r\n",
                  label, negative ? "-" : "", tenths / 10,
                  (tenths % 10) * 10, unit);
    Serial_Print(line);
}

// ADC1 channel 0 (PA0), 12-bit raw range 0..4095.
// Percentage of ADC full scale, NOT calibrated lux or brightness percent.
bool ReadLdr(uint32_t &raw, uint32_t &percent)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return false;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return false;
    }
    raw = HAL_ADC_GetValue(&hadc1);
    const HAL_StatusTypeDef stopped = HAL_ADC_Stop(&hadc1);
    if (stopped != HAL_OK || raw > 4095U) return false;
    percent = (raw * 100U + 2047U) / 4095U; // Rounded to nearest integer.
    return true;
}

void SensorTask(void *argument)
{
    (void)argument;

    Serial_WriteRaw("[SensorTask] started\r\n");

    // One-time startup delay for the DHT22.
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Reference time for periodic sampling.
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        float temperature = 0.0f;
        float humidity = 0.0f;

        // Protect the short microsecond-timed transaction from tick interrupts.
        // TIM4 keeps counting while interrupts are masked.
        taskENTER_CRITICAL();
        const bool valid = ReadDHT22(temperature, humidity);
        taskEXIT_CRITICAL();

        if (valid) {
            PrintReading("Temperature", temperature, "C");
            PrintReading("Humidity", humidity, "%");

        } else {
            Serial_Print("DHT22 read failed: check VCC, GND, PB0 and pull-up.\r\n");
        }

        // ADC polling is outside the DHT critical section.
        // Read the LDR even if the DHT22 transaction failed.
        uint32_t raw = 0U;
        uint32_t percent = 0U;
        if (ReadLdr(raw, percent)) {
            char line[80];
            std::snprintf(line, sizeof(line),
                          "LDR ADC: %lu / 4095 | ADC level: %lu %%\r\n",
                          static_cast<unsigned long>(raw),
                          static_cast<unsigned long>(percent));
            Serial_Print(line);
        } else {
            Serial_Print("LDR ADC read failed: check ADC setup and PA0 wiring.\r\n");
        }
        Serial_Print("\r\n");

                // Wait until the next scheduled 2-second sampling time.
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

void Stop(const char *message)
{
    Serial_WriteRaw(message);
    __disable_irq();
    for (;;) {}
}

} // namespace

int main(void)
{
    Serial_EarlyInit();
    Hardware_Init(); // Includes PB0 and the 1 MHz TIM4 sensor timer.

    if (!RtosObjects_Create()) {
        Stop("RTOS object creation failed\r\n");
    }
    if (xTaskCreate(SensorTask, "SensorTask", 384, nullptr, 1, nullptr) != pdPASS) {
        Stop("SensorTask creation failed\r\n");
    }

    Serial_WriteRaw("DHT22 + LDR serial test: first reading in about 2 seconds.\r\n");
    vTaskStartScheduler();
    Stop("Scheduler failed to start\r\n");
}
