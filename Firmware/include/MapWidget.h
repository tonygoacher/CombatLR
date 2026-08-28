#pragma once

#include "IWidget.h"
#include "Texture.h"
#include "TextureManager.h"

class MapWidget : public IWidget
{
public:

    MapWidget(
        uint32_t id,
        TextureManager& textureManager,
        const GameCoordinate& position,
        const char* filename);

    WidgetCoordinateSpace
        GetCoordinateSpace() const override;

    WidgetPosition
        GetPosition() const override;

    GameCoordinate
        GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;

private:

    GameCoordinate _position;

    Texture* _texture;
};