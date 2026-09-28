#pragma once
#include "app_types.h"
inline DisplayMode Navigate(DisplayMode mode, bool clockwise)
{
    switch (mode) {
        case DisplayMode::TEMPERATURE: return clockwise ? DisplayMode::HUMIDITY : DisplayMode::MOTION;
        case DisplayMode::HUMIDITY: return clockwise ? DisplayMode::LIGHT : DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT: return clockwise ? DisplayMode::MOTION : DisplayMode::HUMIDITY;
        case DisplayMode::MOTION: return clockwise ? DisplayMode::TEMPERATURE : DisplayMode::LIGHT;
    }
    return DisplayMode::TEMPERATURE;
}
inline const char *ModeName(DisplayMode mode)
{
    switch (mode) {
        case DisplayMode::TEMPERATURE: return "TEMPERATURE";
        case DisplayMode::HUMIDITY: return "HUMIDITY";
        case DisplayMode::LIGHT: return "LIGHT";
        case DisplayMode::MOTION: return "MOTION";
    }
    return "TEMPERATURE";
}
