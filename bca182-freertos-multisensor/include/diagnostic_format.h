#pragma once
#include <cstdio>

inline void FormatTemperature(float value, char *text, size_t capacity)
{
    int tenths = static_cast<int>(value * 10.0f + (value >= 0 ? 0.5f : -0.5f));
    const bool negative = tenths < 0;
    if (negative) tenths = -tenths;
    std::snprintf(text, capacity, "%s%d.%02d", negative ? "-" : "",
                  tenths / 10, (tenths % 10) * 10);
}

