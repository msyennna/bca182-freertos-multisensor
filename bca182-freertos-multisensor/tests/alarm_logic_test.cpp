#include "alarm_logic.h"
#include <cstdio>
#include <cmath>
#include <limits>

int main()
{
    struct Case { float temperature; AlarmState expected; };
    const Case cases[] = {
        {-40.0f, AlarmState::LOW_TEMPERATURE},
        {17.9f, AlarmState::LOW_TEMPERATURE},
        {std::nextafter(18.0f, -std::numeric_limits<float>::infinity()), AlarmState::LOW_TEMPERATURE},
        {18.0f, AlarmState::NORMAL},
        {std::nextafter(18.0f, std::numeric_limits<float>::infinity()), AlarmState::NORMAL},
        {25.4f, AlarmState::NORMAL},
        {std::nextafter(30.0f, -std::numeric_limits<float>::infinity()), AlarmState::NORMAL},
        {30.0f, AlarmState::NORMAL},
        {std::nextafter(30.0f, std::numeric_limits<float>::infinity()), AlarmState::HIGH_TEMPERATURE},
        {30.1f, AlarmState::HIGH_TEMPERATURE},
        {80.0f, AlarmState::HIGH_TEMPERATURE}
    };
    unsigned failures = 0;
    for (const Case &test : cases) {
        if (evaluateTemperature(test.temperature) != test.expected) {
            std::printf("FAIL: temperature %.9g\n", test.temperature);
            ++failures;
        }
    }
    if (failures) return 1;
    std::printf("PASS: all 11 alarm decision tests\n");
    return 0;
}
