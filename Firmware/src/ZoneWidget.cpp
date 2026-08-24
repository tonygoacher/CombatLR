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

const bool mapValid =


    Serial.printf(
        "ZoneWidget:\n"
        "  game EN=(%.3f,%.3f)\n"
        "  radius=%.3fm\n"
        "  widget position=(%.3f,%.3f)\n"
        "  widget scale=%.6f\n"
        "  map scale=%.6f\n"
        "  radius pixels=%.3f\n",
        _zone.eastMetres,
        _zone.northMetres,
        _zone.radiusMetres,
        position.x,
        position.y,
        position.scale,
        mapScale,
        radiusPixels);

        Serial.printf(
    "Zone scale: %.6f px/m, radius %.2fm -> %.2f map px -> %.2f screen px\n",
    context.GetMapPixelsPerMetre(),
    _zone.radiusMetres,
    _zone.radiusMetres *
        context.GetMapPixelsPerMetre(),
    _zone.radiusMetres *
        context.GetMapPixelsPerMetre() *
        context.GetMapScale());

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