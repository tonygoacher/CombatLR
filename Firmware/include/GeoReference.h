#pragma once

#include <Arduino.h>

/*
 * Game coordinates are deliberately independent of GPS.
 *
 * X = metres east of Home
 * Y = metres north of Home
 */
struct GameCoordinate
{
    float east = 0.0f;
    float north = 0.0f;
};

/*
 * Defines the GPS origin of a game session and converts GPS positions
 * into the game's local metre-based coordinate system.
 */
class GeoReference
{
public:
    GeoReference();

    void SetHome(
        double latitude,
        double longitude);

    bool IsSet() const;

    double GetHomeLatitude() const;
    double GetHomeLongitude() const;

    GameCoordinate ToGameCoordinate(
        double latitude,
        double longitude) const;

    /*
     * Convert local game coordinates back to GPS.
     * Useful for the operator/map preparation application and testing.
     */
    void ToGPS(
        const GameCoordinate& coordinate,
        double& latitude,
        double& longitude) const;

private:
    double _homeLatitude;
    double _homeLongitude;
    double _metresPerDegreeLongitude;
    bool _set;

    static constexpr double METRES_PER_DEGREE_LATITUDE =
        111320.0;

    static double NormaliseLongitude(double longitude);
};
