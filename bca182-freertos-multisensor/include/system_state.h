#pragma once
#include "app_types.h"
// Pure state decision; MotionTask owns lastMotion and applies event changes.
SystemState evaluateSystemState(bool motion, uint32_t now,
                               uint32_t lastMotion, uint32_t timeout);
