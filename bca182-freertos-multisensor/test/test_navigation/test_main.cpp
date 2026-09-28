#include <unity.h>
#include "navigation.h"

void test_clockwise_moves_to_next_page() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(DisplayMode::HUMIDITY),
        static_cast<int>(Navigate(DisplayMode::TEMPERATURE, true)));
}

void test_clockwise_wraps_around() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(DisplayMode::TEMPERATURE),
        static_cast<int>(Navigate(DisplayMode::MOTION, true)));
}

void test_counterclockwise_moves_to_previous_page() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(DisplayMode::TEMPERATURE),
        static_cast<int>(Navigate(DisplayMode::HUMIDITY, false)));
}

void test_counterclockwise_wraps_around() {
    TEST_ASSERT_EQUAL(
        static_cast<int>(DisplayMode::MOTION),
        static_cast<int>(Navigate(DisplayMode::TEMPERATURE, false)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_clockwise_moves_to_next_page);
    RUN_TEST(test_clockwise_wraps_around);
    RUN_TEST(test_counterclockwise_moves_to_previous_page);
    RUN_TEST(test_counterclockwise_wraps_around);
    return UNITY_END();
}