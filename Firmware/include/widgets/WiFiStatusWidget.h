#pragma once

#include "TFT_eSPI.h"
#include "widgets/Widget.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "Navigation.h"


class WiFiStatusWidget : public Widget
{
public:

    WiFiStatusWidget(
        uint32_t id,
        TextureManager& textureManager,
        const WidgetPosition& position);

    ~WiFiStatusWidget();

    WidgetCoordinateSpace
        GetCoordinateSpace() const override;

    WidgetPosition
        GetPosition() const override;

    GameCoordinate
        GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;

private:

    enum class WifiState
    {
        UnknownWifi,
        GoodWifi,
        BadWifi
    };

    WidgetPosition _position;

    Texture* _ok;
    Texture* _bad;

    TextureManager& _textureManager;

    WifiState _state;
};