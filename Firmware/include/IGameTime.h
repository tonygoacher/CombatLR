#pragma once

#include <stdint.h>

class IGameTime
{
public:
    virtual ~IGameTime() = default;

    virtual uint32_t GetTimeSeconds() const = 0;
};