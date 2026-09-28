#pragma once

#include "app_types.h"

AlarmState evaluateTemperature(float temperature);
DisplayMode nextDisplayMode(DisplayMode current);
DisplayMode previousDisplayMode(DisplayMode current);
SystemState evaluateSystemState(SystemState current, bool inactivityTimedOut, bool motionDetected);
