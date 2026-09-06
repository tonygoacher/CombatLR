

#include "widgets/PlayerHealthWidget.h"
#include "IDisplayContext.h"
#include "Player.h"
#include "TFT_eSPI.h"

#include <algorithm>

PlayerHealthWidget::PlayerHealthWidget(
    int id,
    Player& player,
    int x,
    int y,
    int width,
    int height)
    :
    Widget(id, WidgetType::UI),
    _player(player),
    _position{
        static_cast<float>(x),
        static_cast<float>(y)},
    _width(width),
    _height(height)
{
}

WidgetCoordinateSpace PlayerHealthWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Screen;
}

WidgetPosition PlayerHealthWidget::GetPosition() const
{
    return _position;
}

GameCoordinate PlayerHealthWidget::GetGamePosition() const
{
    GameCoordinate position;
    position.east = 0.0;
    position.north = 0.0;
    return position;
}

void PlayerHealthWidget::Render(IDisplayContext& context)
{
    TFT_eSprite& canvas = context.GetCanvas();

    const WidgetRenderPosition& position =
        context.GetPosition();

    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);

    const int health = std::max(0, std::min(100, _player.GetHealth()));

    canvas.drawRect(
        x,
        y,
        _width,
        _height,
        TFT_WHITE);


    const int innerX = x + 1;
    const int innerY = y + 1;

    const int innerWidth =
        std::max(0, _width - 2);

    const int innerHeight =
        std::max(0, _height - 2);

    if (innerWidth == 0 || innerHeight == 0)
        return;
    
    if (health == 0)
    {
        // Dead: black meter with red diagonal lines.
        canvas.fillRect(
            innerX,
            innerY,
            innerWidth,
            innerHeight,
            TFT_BLACK);

        canvas.drawLine(innerX, innerY, innerX + innerWidth , innerY + innerHeight, TFT_RED);
        canvas.drawLine(innerX + innerWidth, innerY , innerX  , innerY + innerHeight , TFT_RED);        

        return;
    }

    const uint16_t barColour =
        (health <= 20)
            ? TFT_RED
            : TFT_GREEN;

    const int barWidth =
        (innerWidth * health) / 100;

     
    // Clear the meter area first.
    canvas.fillRect(
        innerX,
        innerY,
        innerWidth,
        innerHeight,
        TFT_BLACK); 

    // Health is anchored to the left.
    if (barWidth > 0)
    {
        canvas.fillRect(
            innerX,
            innerY,
            barWidth,
            innerHeight,
            barColour);
    }
}
