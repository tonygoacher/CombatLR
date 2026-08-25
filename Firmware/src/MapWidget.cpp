#include "MapWidget.h"
#include "TFT_eSPI.h"
#include "IDisplayContext.h"

MapWidget::MapWidget(
    TextureManager& textureManager,
    const GameCoordinate& position,
    const char* filename)
    :
    _position(position),
    _texture(
        textureManager.GetTexture(filename))
{
}

WidgetCoordinateSpace
MapWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Map;
}

WidgetPosition
MapWidget::GetPosition() const
{
    return {};
}

GameCoordinate
MapWidget::GetGamePosition() const
{
    return _position;
}

void MapWidget::Render(
    IDisplayContext& context)
{
    if (_texture == nullptr ||
        !_texture->IsLoaded())
    {
        return;
    }

    TFT_eSprite& canvas =
        context.GetCanvas();

    const WidgetRenderPosition& position =
        context.GetPosition();

    /*
     * Map widget textures are authored at 1:1
     * at maximum zoom (zoom 4).
     */
    const float widgetScale =
        (position.zoom ) / 4.0f;

    const uint16_t width =
        _texture->GetWidth();

    const uint16_t height =
        _texture->GetHeight();

    const uint16_t* pixels =
        _texture->GetData();

    /*
     * Render the texture centred on the transformed
     * widget position.
     *
     * Black pixels are transparent.
     */


Serial.printf(
    "MapWidget:\n"
    "  game EN=(%.3f,%.3f)\n"
    "  position=(%.3f,%.3f)\n"
    "  zoom=%.3f\n"
    "  texture=%ux%u\n",
    _position.east,
    _position.north,
    position.x,
    position.y,
    position.zoom,
    _texture->GetWidth(),
    _texture->GetHeight());

    const float scaledWidth =
    static_cast<float>(width) *
    widgetScale;

const float scaledHeight =
    static_cast<float>(height) *
    widgetScale;

const int16_t left =
    static_cast<int16_t>(
        lroundf(
            position.x -
            scaledWidth * 0.5f));

const int16_t top =
    static_cast<int16_t>(
        lroundf(
            position.y -
            scaledHeight * 0.5f));

const int16_t right =
    static_cast<int16_t>(
        lroundf(
            position.x +
            scaledWidth * 0.5f));

const int16_t bottom =
    static_cast<int16_t>(
        lroundf(
            position.y +
            scaledHeight * 0.5f));

for (int16_t screenY = top;
     screenY < bottom;
     ++screenY)
{
    for (int16_t screenX = left;
         screenX < right;
         ++screenX)
    {
        const float sourceX =
            (static_cast<float>(screenX) -
             left) /
            widgetScale;

        const float sourceY =
            (static_cast<float>(screenY) -
             top) /
            widgetScale;

        const uint16_t sourcePixelX =
            static_cast<uint16_t>(
                sourceX);

        const uint16_t sourcePixelY =
            static_cast<uint16_t>(
                sourceY);

        if (sourcePixelX >= width ||
            sourcePixelY >= height)
        {
            continue;
        }

        const uint16_t colour =
            pixels[
                static_cast<size_t>(sourcePixelY) *
                    width +
                sourcePixelX];

        if (colour == TFT_BLACK)
            continue;

        canvas.drawPixel(
            screenX,
            screenY,
            colour);
    }
    }
}