#include "ZoneWidget.h"
#include <TFT_eSPI.h>
#include "IDisplayContext.h"
#include "MapReference.h"

extern MapReference gNavigationMap;

ZoneWidget::ZoneWidget(
    const Zone& zone)
    :
    _zone(zone)
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



Serial.printf(
    "ZoneWidget:\n"
    "  game EN=(%.3f,%.3f)\n"
    "  position=(%.3f,%.3f)\n"
    "  zoom=%.3f\n",
    _zone.eastMetres,
    _zone.northMetres,
    position.x,
    position.y,
    position.zoom);


    canvas.drawCircle(
        static_cast<int16_t>(position.x),
        static_cast<int16_t>(position.y),
        static_cast<int16_t>(radiusPixels),
        TFT_RED);


        canvas.drawCircle(
            static_cast<int16_t>(position.x),
            static_cast<int16_t>(position.y),
            static_cast<int16_t>(radiusPixels)+1,
            TFT_RED);
}