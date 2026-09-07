#pragma once

#include "TFT_eSPI.h"
#include "widgets/Widget.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "Navigation.h"


class UIWidget : public Widget
{
public:

    UIWidget(
        uint32_t id,
        const char * const filename,
        const WidgetPosition& position);

    ~UIWidget();

    WidgetCoordinateSpace
        GetCoordinateSpace() const override;

    WidgetPosition
        GetPosition() const override;

    GameCoordinate
        GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;

private:

    WidgetPosition _position;
    Texture    _texture;
    char    _filename[128];

};