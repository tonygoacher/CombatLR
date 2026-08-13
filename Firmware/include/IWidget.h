#pragma once

#include <stdint.h>

class IDisplayContext;


enum class WidgetCoordinateSpace : uint8_t
{
    Map,
    Screen
};


struct WidgetPosition
{
    float x;
    float y;
};


class IWidget
{
public:

    virtual ~IWidget() = default;


    /*
     * Defines the coordinate system in which the widget
     * position is specified.
     */
    virtual WidgetCoordinateSpace GetCoordinateSpace() const = 0;


    /*
     * Return the widget's position.
     *
     * Map widgets:
     *     coordinates relative to the map origin.
     *
     * Screen widgets:
     *     coordinates in screen pixels.
     */
    virtual WidgetPosition GetPosition() const = 0;


    /*
     * Render the widget.
     *
     * The supplied context contains the final screen position
     * calculated by the display.
     *
     * The widget must NOT perform map rotation itself.
     */
    virtual void Render(
        IDisplayContext& context) = 0;
};