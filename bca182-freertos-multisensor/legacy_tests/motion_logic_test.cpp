#include "motion_logic.h"
#include <cstdio>
int main() {
    struct Case {bool motion; uint32_t now,last; SystemState expected;};
    const Case cases[] = {
        {false,0,0,SystemState::ACTIVE},
        {false,14999,0,SystemState::ACTIVE},
        {false,15000,0,SystemState::INACTIVE},
        {false,20000,0,SystemState::INACTIVE},
        {true,20000,0,SystemState::ACTIVE},
        {false,34999,20000,SystemState::ACTIVE},
        {false,35000,20000,SystemState::INACTIVE},
        {false,50,0xfffffff0U,SystemState::ACTIVE},
        {false,14984,0xfffffff0U,SystemState::INACTIVE}
    };
    for (const auto &c:cases) if(evaluateSystemState(c.motion,c.now,c.last,15000)!=c.expected) return 1;
    std::puts("PASS: 9 motion/state cases, including timeout boundary and clock wrap");
}
