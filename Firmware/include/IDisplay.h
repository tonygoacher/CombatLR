#pragma once

#include <stdint.h>
#include "GameCoordinate.h"

class IWidget;


class IDisplay
{
public:

    virtual ~IDisplay() = default;


    virtual bool Init() = 0;

    virtual bool ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude) =0;

    /*
     * Dynamic map rotation in degrees.
     *
     * This rotates the map and the positions of Map widgets,
     * but does NOT rotate the widgets themselves.
     */
    virtual void SetRotation(float angle) = 0;

    virtual float GetRotation() const = 0;


    /*
     * Map zoom factor.
     *
     * 1.0 = normal
     * 2.0 = twice as large
     */
    virtual void SetMapZoom(float zoom) = 0;

    virtual float GetMapZoom() const = 0;

    virtual void RemovedWidget(uint32_t widgetId) = 0;

    virtual void SetWidgetColour(uint32_t widgetId, uint32_t colour) = 0;

    virtual void SetZoneGeometry( uint32_t zoneId,  GameCoordinate newPos,  float radius) = 0;
    /*
     * Add/remove widgets.
     *
     * Display does not take ownership of the widget.
     */
    virtual uint16_t AddWidget(
        IWidget* widget) = 0;

    virtual bool RemoveWidget(
        uint16_t widgetId) = 0;


    /*
     * Render the current display.
     */
    virtual void Update(
    const GameCoordinate& playerPosition,
    float heading,
    float zoom,
    bool playerVisible) = 0;


    virtual void Clear() = 0;

};