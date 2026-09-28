#pragma once
#include "app_types.h"

// Input must be a valid, finite temperature in Celsius.
// Below 18 C: LOW; above 30 C: HIGH; boundaries inclusive: NORMAL.
// No HAL, FreeRTOS, I/O, mutable state, or buzzer side effects.
AlarmState evaluateTemperature(float temperature);
