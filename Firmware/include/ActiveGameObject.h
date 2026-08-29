#pragma once
#include "GameCoordinate.h"
#include "IWidget.h"

struct ActiveGameObject
{
    uint32_t id;
    GameCoordinate position;
    WidgetType type;
    bool claimed;
};