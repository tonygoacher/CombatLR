#pragma once

#include "GameCoordinate.h"
#include <stdint.h>

class IZoneEvent
{
public:
    virtual void ZoneImminent(GameCoordinate newZonePos, int zoneIndexInGamePlan, uint32_t timeToActivation) =0;
    virtual void ZoneExpired(GameCoordinate newZonePos, int zoneIndexInGamePlan) = 0;
};