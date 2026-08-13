#pragma once

#include <stdint.h>

#include <TFT_eSPI.h>


struct WidgetRenderPosition
{
    float x;
    float y;

    /*
     * Useful later for map objects such as zones.
     */
    float scale;
};


class IDisplayContext
{
public:

    virtual ~IDisplayContext() = default;


    virtual TFT_eSprite& GetCanvas() = 0;


    virtual const WidgetRenderPosition&
        GetPosition() const = 0;
};