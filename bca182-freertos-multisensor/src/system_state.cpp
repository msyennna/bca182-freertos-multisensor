#include "system_state.h"

SystemState evaluateSystemState(bool motion, uint32_t now,
                                       uint32_t lastMotion, uint32_t timeout)
{
    return motion || static_cast<uint32_t>(now - lastMotion) < timeout
        ? SystemState::ACTIVE : SystemState::INACTIVE;
}
