#include "ZoneWidget.h"
#include <TFT_eSPI.h>
#include "IDisplayContext.h"
#include "MapReference.h"

extern MapReference gNavigationMap;

ZoneWidget::ZoneWidget(uint32_t id, const Zone& zone, uint32_t colour)
    :
    IWidget(id, WidgetType::Zone),
    _zone(zone),
    _colour(colour)
{
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

    position.east = _zone.eastMetres;
    position.north = _zone.northMetres;

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
        static_cast<float>(_zone.radiusMetres) *
        context.GetMapPixelsPerMetre() *
        context.GetMapScale();

        float mapX = 0.0f;
float mapY = 0.0f;


GameCoordinate zonePosition;

zonePosition.east =
    _zone.eastMetres;

zonePosition.north =
    _zone.northMetres;



    canvas.drawSmoothArc(
        static_cast<int16_t>(position.x),
        static_cast<int16_t>(position.y),
        static_cast<int16_t>(radiusPixels),
        static_cast<int16_t>(radiusPixels)+4,
        0,360,
        _colour, TFT_BLACK);


}