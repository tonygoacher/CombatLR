#pragma once

#include <stdint.h>

#include "Widget.h"
#include "graphics/Texture.h"
#include "graphics/IDisplayContext.h"

class TextureManager;


class TextureWidget : public Widget
{
public:

    TextureWidget(
        uint32_t id,
        TextureManager& textureManager,
        const WidgetPosition& position,
        const char* filename);

    ~TextureWidget();

    WidgetCoordinateSpace
        GetCoordinateSpace() const override;

    WidgetPosition
        GetPosition() const override;

    GameCoordinate
        GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;

    bool SetTexture(
        const char* filename);

private:

    WidgetPosition _position;

    Texture* _texture;

    TextureManager& _textureManager;
};