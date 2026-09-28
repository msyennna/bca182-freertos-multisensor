#include <cstdio>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"
#include "ssd1306.h"
#include "encoder.h"
#include "navigation.h"
#include "alarm_logic.h"

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

// The queue copies this task-local structure into each consumer's mailbox.
void SensorTask(void *argument)
{
    (void)argument;
    Serial_Print("[SensorTask] started\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000)); // One-time sensor startup delay.
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        SensorData data = {};
        taskENTER_CRITICAL();
        const bool dhtOk = ReadDHT22(data.temperature, data.humidity);
        taskEXIT_CRITICAL();

        uint32_t raw = 0U;
        uint32_t percent = 0U;
        const bool ldrOk = ReadLdr(raw, percent);
        data.lightLevel = static_cast<int>(percent); // ADC full-scale %, not lux.
        data.motionDetected = false; // Placeholder: PIR is not integrated yet.

        if (dhtOk && ldrOk) {
            // Separate length-one queues give both consumers their own copy.
            // A slow consumer gets the latest sample, not a historical backlog.
            xQueueOverwrite(displaySensorQueue, &data);
            xQueueOverwrite(alarmSensorQueue, &data);
        } else {
            // The required four-field struct has no validity flags.
            // Do not publish zeros as measurements if acquisition failed.
            Serial_Print("[SensorTask] Read failed; sample not published.\r\n");
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

void FormatTemperature(float value, char *text, size_t capacity)
{
    int tenths = static_cast<int>(value * 10.0f + (value >= 0 ? 0.5f : -0.5f));
    const bool negative = tenths < 0;
    if (negative) tenths = -tenths;
    std::snprintf(text, capacity, "%s%d.%02d", negative ? "-" : "",
                  tenths / 10, (tenths % 10) * 10);
}

// Serial-only consumers demonstrate queue reception before OLED/alarm integration.
void PrintReceived(const char *consumer, const SensorData &data)
{
    char temperature[20];
    char humidity[20];
    char line[180];
    FormatTemperature(data.temperature, temperature, sizeof(temperature));
    FormatTemperature(data.humidity, humidity, sizeof(humidity));
    std::snprintf(line, sizeof(line),
                  "[%s] T=%s C | H=%s %% | ADC level=%d %% | Motion=%s (PIR not added)\r\n",
                  consumer, temperature, humidity, data.lightLevel,
                  data.motionDetected ? "YES" : "NO");
    // One mutex-protected call keeps each consumer's whole line together.
    Serial_Print(line);
}

// Called only from DisplayTask: all OLED access stays with one owner.
bool DrawSelectedPage(DisplayMode mode, const SensorData &data, bool fresh)
{
    char value[32];
    SSD1306_Clear();
    SSD1306_DrawText(0, 0, "ROOM MONITOR");
    SSD1306_DrawText(0, 2, ModeName(mode));
    if (mode == DisplayMode::MOTION) {
        SSD1306_DrawText(0, 4, "PIR NOT ADDED");
    } else if (!fresh) {
        SSD1306_DrawText(0, 4, "NO FRESH DATA");
    } else {
        if (mode == DisplayMode::LIGHT) {
            std::snprintf(value, sizeof(value), "%d %%", data.lightLevel);
        } else {
            const float number = mode == DisplayMode::TEMPERATURE ? data.temperature : data.humidity;
            int tenths = static_cast<int>(number * 10.0f + (number >= 0 ? 0.5f : -0.5f));
            const bool negative = tenths < 0;
            if (negative) tenths = -tenths;
            std::snprintf(value, sizeof(value), "%s%d.%d %s", negative ? "-" : "",
                          tenths / 10, tenths % 10,
                          mode == DisplayMode::TEMPERATURE ? "C" : "%");
        }
        SSD1306_DrawText(0, 4, value);
    }
    if (mode == DisplayMode::LIGHT) SSD1306_DrawText(0, 6, "ADC LEVEL - NOT LUX");
    return SSD1306_Update();
}

void DisplayTask(void *argument)
{
    (void)argument;
    vTaskDelay(pdMS_TO_TICKS(100));
    bool oledReady = false, haveSample = false, wasFresh = false;
    SensorData received = {};
    DisplayMode mode = DisplayMode::TEMPERATURE;
    TickType_t lastSample = 0;
    for (;;) {
        bool changed = false;
        if (!oledReady) {
            oledReady = SSD1306_Init();
            if (!oledReady) {
                Serial_Print("[DisplayTask] OLED init failed\r\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            Serial_Print("[DisplayTask] OLED ready\r\n");
            changed = true;
        }
        SensorData incoming;
        if (xQueueReceive(displaySensorQueue, &incoming, 0) == pdPASS) {
            received = incoming;
            haveSample = true;
            lastSample = xTaskGetTickCount();
            changed = true;
            PrintReceived("DisplayTask", received);
        }
        DisplayMode requested;
        if (xQueueReceive(displayModeQueue, &requested, 0) == pdPASS) {
            if (mode != requested) changed = true;
            mode = requested;
        }
        const bool fresh = haveSample &&
            static_cast<TickType_t>(xTaskGetTickCount() - lastSample) < pdMS_TO_TICKS(5000);
        if (fresh != wasFresh) changed = true;
        wasFresh = fresh;
        if (changed) {
            oledReady = DrawSelectedPage(mode, received, fresh);
            if (!oledReady) Serial_Print("[DisplayTask] OLED update failed\r\n");
        }
        // Respond to navigation without waiting for the next 2-second sample.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void AlarmTask(void *argument)
{
    (void)argument;
    SensorData received = {};
    for (;;) {
        if (xQueueReceive(alarmSensorQueue, &received, portMAX_DELAY) == pdPASS) {
            const AlarmState state = evaluateTemperature(received.temperature);
            const char *name = "NORMAL";
            switch (state) {
                case AlarmState::NORMAL: name = "NORMAL"; break;
                case AlarmState::LOW_TEMPERATURE: name = "LOW_TEMPERATURE"; break;
                case AlarmState::HIGH_TEMPERATURE: name = "HIGH_TEMPERATURE"; break;
            }
            char temperature[20];
            char line[100];
            FormatTemperature(received.temperature, temperature, sizeof(temperature));
            std::snprintf(line, sizeof(line), "[AlarmTask] T=%s C | State=%s\r\n",
                          temperature, name);
            Serial_Print(line);
            // Hardware action is intentionally separate; add buzzer control later.
        }
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

    if (xTaskCreate(DisplayTask, "DisplayTask", 512, nullptr, 1, nullptr) != pdPASS ||
        xTaskCreate(AlarmTask, "AlarmTask", 384, nullptr, 1, nullptr) != pdPASS) {
        Stop("Consumer task creation failed\r\n");
    }
    if (xTaskCreate(InputTask, "InputTask", 256, nullptr, 1, nullptr) != pdPASS) {
        Stop("InputTask creation failed\r\n");
    }
    Serial_WriteRaw("Encoder + OLED test: first reading in about 2 seconds.\r\n");
    vTaskStartScheduler();
    Stop("Scheduler failed to start\r\n");
}
