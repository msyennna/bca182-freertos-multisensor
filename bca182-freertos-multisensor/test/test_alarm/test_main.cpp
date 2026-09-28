#include <unity.h>
#include "alarm_logic.h"

// Compile the pure logic with this test, without STM32 hardware code.
#include "../../src/alarm_logic.cpp"

void test_below_lower_limit() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(AlarmState::LOW_TEMPERATURE),
        static_cast<int>(evaluateTemperature(17.9f)));
}

void test_exactly_lower_limit() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(AlarmState::NORMAL),
        static_cast<int>(evaluateTemperature(18.0f)));
}

void test_normal_temperature() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(AlarmState::NORMAL),
        static_cast<int>(evaluateTemperature(25.0f)));
}

void test_exactly_upper_limit() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(AlarmState::NORMAL),
        static_cast<int>(evaluateTemperature(30.0f)));
}

void test_above_upper_limit() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(AlarmState::HIGH_TEMPERATURE),
        static_cast<int>(evaluateTemperature(30.1f)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_below_lower_limit);
    RUN_TEST(test_exactly_lower_limit);
    RUN_TEST(test_normal_temperature);
    RUN_TEST(test_exactly_upper_limit);
    RUN_TEST(test_above_upper_limit);
    return UNITY_END();
}