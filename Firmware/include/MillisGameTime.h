#pragma once

#include "IGameTime.h"

class MillisGameTime : public IGameTime
{
public:
    uint32_t GetTimeSeconds() const override;
};