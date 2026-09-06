#pragma once

#include "Widget.h"

class Player;

class PlayerHealthWidget : public Widget
{
public:
    PlayerHealthWidget(
        int id,
        Player& player,
        int x,
        int y,
        int width,
        int height);

    WidgetCoordinateSpace GetCoordinateSpace() const override;
    WidgetPosition GetPosition() const override;
    GameCoordinate GetGamePosition() const override;

    void Render(IDisplayContext& context) override;

private:
    Player& _player;
    WidgetPosition _position;
    int _width;
    int _height;
};
