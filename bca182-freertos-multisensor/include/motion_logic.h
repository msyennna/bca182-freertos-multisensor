#pragma once
#include "app_types.h"
// Unsigned elapsed ticks handle a single timer wrap naturally.
inline SystemState evaluateSystemState(bool motion, uint32_t now,
                                       uint32_t lastMotion, uint32_t timeout)
{
    return motion || static_cast<uint32_t>(now - lastMotion) < timeout
        ? SystemState::ACTIVE : SystemState::INACTIVE;
}
