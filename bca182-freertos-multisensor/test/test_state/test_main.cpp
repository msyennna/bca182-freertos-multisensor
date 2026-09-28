#include <unity.h>
#include "system_state.h"

#include "../../src/system_state.cpp"

constexpr uint32_t TIMEOUT = 15000U;

void test_active_before_timeout() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(SystemState::ACTIVE),
        static_cast<int>(evaluateSystemState(false, 14999U, 0U, TIMEOUT)));
}

void test_inactive_at_timeout() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(SystemState::INACTIVE),
        static_cast<int>(evaluateSystemState(false, 15000U, 0U, TIMEOUT)));
}

void test_inactive_without_motion() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(SystemState::INACTIVE),
        static_cast<int>(evaluateSystemState(false, 20000U, 0U, TIMEOUT)));
}

void test_motion_reactivates_system() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(SystemState::ACTIVE),
        static_cast<int>(evaluateSystemState(true, 20000U, 0U, TIMEOUT)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_active_before_timeout);
    RUN_TEST(test_inactive_at_timeout);
    RUN_TEST(test_inactive_without_motion);
    RUN_TEST(test_motion_reactivates_system);
    return UNITY_END();
}