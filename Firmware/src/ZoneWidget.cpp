#include "ZoneWidget.h"
#include <TFT_eSPI.h>
#include "IDisplayContext.h"
#include "MapReference.h"



ZoneWidget::ZoneWidget(
    uint32_t id,
    const GameCoordinate& position,
    const uint16_t radius,
    uint32_t colour)
    :
    IWidget(id, WidgetType::Zone),
    _radius(radius),
    _position(position)
{
    _colour = colour;
}

WidgetPosition ZoneWidget::GetPosition() const
{
    return {};
}

WidgetCoordinateSpace
ZoneWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Map;
}

GameCoordinate
ZoneWidget::GetGamePosition() const
{
    GameCoordinate position;

    position.east = _position.east;
    position.north = _position.north;

    return position;
}

void ZoneWidget::Render(
    IDisplayContext& context)
{
    TFT_eSprite& canvas =
        context.GetCanvas();

    const WidgetRenderPosition& position =
        context.GetPosition();

    const float mapScale =
        context.GetMapScale();

    const float radiusPixels =
        static_cast<float>(_radius) *
        context.GetMapPixelsPerMetre() *
        context.GetMapScale();

    canvas.drawSmoothArc(
        static_cast<int16_t>(position.x),
        static_cast<int16_t>(position.y),
        static_cast<int16_t>(radiusPixels),
        static_cast<int16_t>(radiusPixels)+4,
        0,360,
        _colour, TFT_BLACK);


}