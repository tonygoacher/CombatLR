#pragma once

#include "GameCoordinate.h"
#include <stdint.h>

class IZoneEvent
{
public:
    virtual void ZoneMoveImminent(
        GameCoordinate newZonePos,
        int zoneIndexInGamePlan,
        uint32_t timeToActivation) {};

    virtual void ZoneStartedMoving(
        GameCoordinate newZonePos,
        int zoneIndexInGamePlan) {};

    virtual void NewZoneActive(
        GameCoordinate newZonePos,
        int zoneIndexInGamePlan) {};
};
