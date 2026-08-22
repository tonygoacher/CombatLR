#include "MillisGameTime.h"

#include <Arduino.h>

uint32_t MillisGameTime::GetTimeSeconds() const
{
    return millis() / 1000;
}