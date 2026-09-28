#include "display.h"
#include "ssd1306.h"
#include "navigation.h"
#include "diagnostic_format.h"
#include <cstdio>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"

namespace {
void PrintReceived(const char *consumer, const SensorData &data)
{
    char temperature[20];
    char humidity[20];
    char line[180];
    FormatTemperature(data.temperature, temperature, sizeof(temperature));
    FormatTemperature(data.humidity, humidity, sizeof(humidity));
    std::snprintf(line, sizeof(line),
                  "[%s] T=%s C | H=%s %% | ADC level=%d %% | Motion=%s\r\n",
                  consumer, temperature, humidity, data.lightLevel,
                  data.motionDetected ? "YES" : "NO");
    // One mutex-protected call keeps each consumer's whole line together.
    Serial_Print(line);
}

bool DrawSelectedPage(DisplayMode mode, const SensorData &data, bool fresh, bool alarmActive)
{
    char value[32];
    SSD1306_Clear();
    SSD1306_DrawText(0, 0, "ROOM MONITOR");
    SSD1306_DrawText(0, 2, ModeName(mode));
    if (mode == DisplayMode::MOTION) {
        SSD1306_DrawText(0, 4, data.motionDetected ? "DETECTED" : "NONE");
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
    if (alarmActive) SSD1306_DrawText(0, 7, "TEMP ALARM");
    return SSD1306_Update();
}


}

void DisplayTask(void *argument)
{
    (void)argument;
    vTaskDelay(pdMS_TO_TICKS(100));
    bool oledReady = false, haveSample = false, wasFresh = false;
    bool displaySleeping = false;
    bool previousAlarm = false;
    SensorData received = {};
    DisplayMode mode = DisplayMode::TEMPERATURE;
    TickType_t lastSample = 0;
    for (;;) {
        bool changed = false;
        if ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) == 0) {
            if (oledReady && !displaySleeping) {
                SSD1306_Clear();
                const bool blanked = SSD1306_Update();
                const bool off = SSD1306_DisplayOff();
                if (!blanked || !off) Serial_Print("[DisplayTask] OLED sleep I2C error\r\n");
                Serial_Print("[DisplayTask] OLED off; waiting for motion\r\n");
            }
            displaySleeping = true;
            // No periodic framebuffer/I2C work while inactive.
            xEventGroupWaitBits(systemEvents, EVENT_ACTIVE, pdFALSE, pdTRUE, portMAX_DELAY);
            continue;
        }
        if (displaySleeping) {
            // Reinitialize on wake, which turns the display on and refreshes it.
            displaySleeping = false;
            oledReady = false;
            haveSample = false;
            Serial_Print("[DisplayTask] Resuming OLED\r\n");
        }
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
        const bool liveMotion = (xEventGroupGetBits(systemEvents) & EVENT_MOTION) != 0;
        if (received.motionDetected != liveMotion) {
            received.motionDetected = liveMotion;
            if (mode == DisplayMode::MOTION) changed = true;
        }
        const bool fresh = haveSample &&
            static_cast<TickType_t>(xTaskGetTickCount() - lastSample) < pdMS_TO_TICKS(5000);
        if (fresh != wasFresh) changed = true;
        wasFresh = fresh;
        const bool alarmActive = (xEventGroupGetBits(systemEvents) & EVENT_ALARM) != 0;
        if (alarmActive != previousAlarm) changed = true;
        previousAlarm = alarmActive;
        if (changed) {
            oledReady = DrawSelectedPage(mode, received, fresh, alarmActive);
            if (!oledReady) Serial_Print("[DisplayTask] OLED update failed\r\n");
        }
        // Respond to navigation without waiting for the next 2-second sample.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

