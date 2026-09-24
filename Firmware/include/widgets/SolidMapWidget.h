#pragma once

#include "MapWidget.h"
#include "graphics/IDisplayContext.h"

class SolidMapWidget : public MapWidget
{
public:

    SolidMapWidget(
        uint32_t id,
        TextureManager& textureManager,
        const GameCoordinate& position,
        const char* filename,
        PickupTypes pickupType);

    void Render(
        IDisplayContext& context) override;
};