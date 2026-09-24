#include "widgets/SolidMapWidget.h"
#include "graphics/SpriteManager.h"

SolidMapWidget::SolidMapWidget(
    uint32_t id,
    TextureManager& textureManager,
    const GameCoordinate& position,
    const char* filename,
    PickupTypes pickupType)
    :
    MapWidget(
        id,
        textureManager,
        position,
        filename,
        pickupType)
{
}


void SolidMapWidget::Render(
    IDisplayContext& context)
{
    if (_texture == nullptr ||
        !_texture->IsLoaded())
    {
        return;
    }

    SpriteSet* sprites =
        context.GetSpriteManager().Get(
            context.GetCanvas(),
            _filename,
            *_texture);

    if (sprites == nullptr)
    {
        return;
    }

    TFT_eSprite* sprite;

    if (context.GetPosition().zoom < 2.0f)
    {
        sprite = sprites->small;
    }
    else
    {
        sprite = sprites->full;
    }

    if (sprite == nullptr)
    {
        return;
    }

    const int16_t left =
        static_cast<int16_t>(
            lroundf(
                context.GetPosition().x -
                sprite->width() * 0.5f));

    const int16_t top =
        static_cast<int16_t>(
            lroundf(
                context.GetPosition().y -
                sprite->height() * 0.5f));

    sprite->pushToSprite(
        &context.GetCanvas(),
        left,
        top);
}