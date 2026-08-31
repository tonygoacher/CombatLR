#pragma once

#include <stdint.h>
#include "GeoReference.h"

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

enum class WidgetType
{
    Pickup,
    Zone,
    GPS
};


class IWidget
{
public:

    IWidget(
        int id,
        WidgetType type)
        :
        _id(id),
        _visible(true),
        _type(type)
    {
    }

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

    virtual GameCoordinate GetGamePosition() const = 0;

    void SetColour(uint32_t colour){_colour = colour;}

    bool GetVisible() { return _visible;}
    void SetVisible(bool visible){ _visible = visible;}

    virtual void  SetGamePosition(GameCoordinate newPos) { };
    virtual void  SetRadius(uint16_t newRadius) { };


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

    int GetId() const
    {
        return _id;
    }

    WidgetType GetType() const
    {
        return _type;
    }

protected:
    const int _id;
    const WidgetType _type;    
    bool _visible;
    uint32_t _colour;


};