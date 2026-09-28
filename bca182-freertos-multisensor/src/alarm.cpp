#include "alarm.h"
#include "alarm_logic.h"
#include "diagnostic_format.h"
#include <cstdio>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "serial_log.h"
#include "rtos_objects.h"

void AlarmTask(void *argument)
{
    (void)argument;
    SensorData received = {};
    bool buzzerOn = false;
    Buzzer_Set(false);
    for (;;) {
        if ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) == 0) {
            xEventGroupClearBits(systemEvents, EVENT_ALARM);
            Buzzer_Set(false);
            if (buzzerOn) Serial_Print("[Buzzer] OFF: system INACTIVE\r\n");
            buzzerOn = false;
            Serial_Print("[AlarmTask] Paused while INACTIVE\r\n");
            xEventGroupWaitBits(systemEvents, EVENT_ACTIVE, pdFALSE, pdTRUE, portMAX_DELAY);
        }
        if (xQueueReceive(alarmSensorQueue, &received, pdMS_TO_TICKS(100)) == pdPASS) {
            if ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) == 0) continue;
            const AlarmState state = evaluateTemperature(received.temperature);
            const bool alarmActive = state != AlarmState::NORMAL;
            const bool previouslySet = (xEventGroupGetBits(systemEvents) & EVENT_ALARM) != 0;
            if (alarmActive) {
                xEventGroupSetBits(systemEvents, EVENT_ALARM);
            } else {
                xEventGroupClearBits(systemEvents, EVENT_ALARM);
            }
            if (alarmActive != previouslySet) {
                Serial_Print(alarmActive ? "[Event] EVENT_ALARM SET\r\n"
                                         : "[Event] EVENT_ALARM CLEARED\r\n");
            }
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
            const bool shouldSound = alarmActive &&
                ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) != 0);
            Buzzer_Set(shouldSound);
            if (shouldSound != buzzerOn) {
                Serial_Print(shouldSound ? "[Buzzer] ON: temperature alarm\r\n"
                                         : "[Buzzer] OFF\r\n");
            }
            buzzerOn = shouldSound;
        }
    }
}

