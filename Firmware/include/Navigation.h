#pragma once

#include <Arduino.h>



/*
 * Application-level navigation data.
 *
 * Position is WGS84 latitude/longitude.
 * Heading is degrees clockwise from magnetic north.
 * Course is degrees clockwise from true north, as supplied by GNSS.
 */
struct NavigationData
{
    bool positionValid = false;
    bool headingValid = false;

    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0f;

    float speed = 0.0f;      // m/s
    float course = 0.0f;     // degrees, true north
    float heading = 0.0f;    // degrees, magnetic north

    uint8_t satellites = 0;
};

/*
 * Common interface used by the game.
 *
 * The game deliberately knows nothing about NMEA, UBX, serial ports,
 * or the particular GNSS/magnetometer hardware.
 */
class INavigation
{
public:
    virtual ~INavigation() = default;

    virtual bool Begin() = 0;
    virtual bool Update() = 0;

    virtual NavigationData GetData() const = 0;

    bool HasPosition() const
    {
        return GetData().positionValid;
    }

    bool HasHeading() const
    {
        return GetData().headingValid;
    }
};
