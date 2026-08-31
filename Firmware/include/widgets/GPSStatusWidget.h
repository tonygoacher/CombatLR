#pragma once

#include "TFT_eSPI.h"
#include "widgets/Widget.h"
#include "Texture.h"
#include "TextureManager.h"
#include "Navigation.h"


class GPSStatusWidget : public Widget
{
public:

    GPSStatusWidget(
        uint32_t id,
        TextureManager& textureManager,
        const WidgetPosition& position,
        INavigation& navigation);

    ~GPSStatusWidget();

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

    Texture* _okSat;
    Texture* _badSat;

    INavigation& _navigation;
    TextureManager& _textureManager;
};