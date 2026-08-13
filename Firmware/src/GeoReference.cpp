#include "GeoReference.h"

#include <math.h>
#include "WorldConstants.h"

GeoReference::GeoReference()
    : _homeLatitude(0.0),
      _homeLongitude(0.0),
      _metresPerDegreeLongitude(0.0),
      _set(false)
{
}

void GeoReference::SetHome(
    double latitude,
    double longitude)
{
    _homeLatitude = latitude;
    _homeLongitude = NormaliseLongitude(longitude);

    /*
     * For the relatively small game area, treating the Earth locally as
     * a plane is sufficiently accurate. Longitude degrees get shorter
     * as latitude increases, hence the cosine correction.
     */
    _metresPerDegreeLongitude =
        METRES_PER_DEGREE_LATITUDE *
        cos(_homeLatitude * WorldConstants::kDegToRad);

    _set = (_metresPerDegreeLongitude > 0.001);
}

bool GeoReference::IsSet() const
{
    return _set;
}

double GeoReference::GetHomeLatitude() const
{
    return _homeLatitude;
}

double GeoReference::GetHomeLongitude() const
{
    return _homeLongitude;
}

GameCoordinate GeoReference::ToGameCoordinate(
    double latitude,
    double longitude) const
{
    GameCoordinate result;

    if (!_set)
        return result;

    /*
     * X increases east.
     * Y increases north.
     */
    const double deltaLongitude =
        NormaliseLongitude(longitude - _homeLongitude);

    const double deltaLatitude =
        latitude - _homeLatitude;

    result.east =
        static_cast<float>(
            deltaLongitude *
            _metresPerDegreeLongitude);

    result.north =
        static_cast<float>(
            deltaLatitude *
            METRES_PER_DEGREE_LATITUDE);

    return result;
}

void GeoReference::ToGPS(
    const GameCoordinate& coordinate,
    double& latitude,
    double& longitude) const
{
    if (!_set)
    {
        latitude = 0.0;
        longitude = 0.0;
        return;
    }

    latitude =
        _homeLatitude +
        static_cast<double>(coordinate.north) /
            METRES_PER_DEGREE_LATITUDE;

    longitude =
        _homeLongitude +
        static_cast<double>(coordinate.east) /
            _metresPerDegreeLongitude;

    longitude = NormaliseLongitude(longitude);
}

double GeoReference::NormaliseLongitude(double longitude)
{
    while (longitude > 180.0)
        longitude -= 360.0;

    while (longitude < -180.0)
        longitude += 360.0;

    return longitude;
}
