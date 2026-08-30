#pragma once
#include "GameCoordinate.h"
#include "IWidget.h"
#include "PickupTypes.h"

struct ActiveGameObject
{
    uint32_t id;
    GameCoordinate position;
    PickupTypes pickupType;
    WidgetType type;
    bool claimed;
};