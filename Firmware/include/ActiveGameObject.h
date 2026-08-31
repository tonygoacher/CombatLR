#pragma once
#include "GameCoordinate.h"
#include "widgets/Widget.h"
#include "PickupTypes.h"

struct ActiveGameObject
{
    uint32_t id;
    GameCoordinate position;
    PickupTypes pickupType;
    WidgetType type;
    bool claimed;
};