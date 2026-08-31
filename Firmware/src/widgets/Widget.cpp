#include "TFT_eSPI.h"
#include "widgets/Widget.h"



bool Widget::LoadSprite(
    TFT_eSprite& sprite,
    const uint16_t* pixels,
    int width,
    int height)
{
    sprite.setColorDepth(16);

    if (sprite.createSprite(width, height) == nullptr)
    {
        return false;
    }

    sprite.pushImage(
        0,
        0,
        width,
        height,
        pixels);

    return true;
}