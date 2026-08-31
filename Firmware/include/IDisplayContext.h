#pragma once

#include <stdint.h>


#include "Texture.h"


struct WidgetRenderPosition
{
    float x;
    float y;

    /*
     * Useful later for map objects such as zones.
     */ 
    float scale;
    float zoom;
};


class IDisplayContext
{
public:

    virtual ~IDisplayContext() = default;


    virtual TFT_eSprite& GetCanvas() = 0;


    virtual const WidgetRenderPosition&
        GetPosition() const = 0;

    virtual float GetMapScale() const = 0;

     virtual float GetMapPixelsPerMetre() const = 0;

     virtual void DrawTexture(
            const Texture& texture,
            uint16_t transparentColour) = 0;
};