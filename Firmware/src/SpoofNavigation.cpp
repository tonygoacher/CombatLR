#include "SpoofNavigation.h"

#include <math.h>
#include "WorldConstants.h"


SpoofNavigation::SpoofNavigation()
{
    _data.positionValid = true;
    _data.headingValid = true;
    _data.latitude = 0.0;
    _data.longitude = 0.0;
    _data.altitude = 0.0f;
    _data.speed = 0.0f;
    _data.course = 0.0f;
    _data.heading = 0.0f;
    _data.satellites = 12;
}

bool SpoofNavigation::Begin()
{
    return true;
}

void SpoofNavigation::Update()
{
    // Nothing to do. The test position is set explicitly.
}

NavigationData SpoofNavigation::GetData() const
{
    return _data;
}

void SpoofNavigation::SetPosition(
    double latitude,
    double longitude)
{
    _data.latitude = latitude;
    _data.longitude = NormaliseLongitude(longitude);
    _data.positionValid = true;
}

void SpoofNavigation::SetPosition(
    double latitude,
    double longitude,
    float altitude)
{
    SetPosition(latitude, longitude);
    _data.altitude = altitude;
}

void SpoofNavigation::SetPositionRelative(
    double homeLatitude,
    double homeLongitude,
    float eastMeters,
    float northMeters)
{
    /*
     * Equirectangular approximation. This is more than adequate for the
     * relatively small playing areas we're dealing with.
     */
    const double latitudeRadians =
        homeLatitude * WorldConstants::kDegToRad;

    const double metresPerDegreeLongitude =
        WorldConstants::kMetresPerDegreeLatitude *
        cos(latitudeRadians);

    if (metresPerDegreeLongitude == 0.0)
        return;

const double latitude =
    homeLatitude +
    static_cast<double>(northMeters) /
       WorldConstants::kMetresPerDegreeLatitude;

    const double longitude =
        homeLongitude +
        static_cast<double>(eastMeters) /
            metresPerDegreeLongitude;

    SetPosition(latitude, longitude);
}

void SpoofNavigation::SetHeading(float headingDegrees)
{
    _data.heading =
        NormaliseHeading(headingDegrees);

    _data.headingValid = true;
}

void SpoofNavigation::SetCourse(float courseDegrees)
{
    _data.course =
        NormaliseHeading(courseDegrees);
}

void SpoofNavigation::SetSpeed(float speedMps)
{
    _data.speed = speedMps;
}

void SpoofNavigation::SetPositionValid(bool valid)
{
    _data.positionValid = valid;
}

void SpoofNavigation::SetHeadingValid(bool valid)
{
    _data.headingValid = valid;
}

void SpoofNavigation::SetSatellites(uint8_t satellites)
{
    _data.satellites = satellites;
}

double SpoofNavigation::NormaliseLongitude(double longitude)
{
    while (longitude > 180.0)
        longitude -= 360.0;

    while (longitude < -180.0)
        longitude += 360.0;

    return longitude;
}

float SpoofNavigation::NormaliseHeading(float heading)
{
    while (heading >= 360.0f)
        heading -= 360.0f;

    while (heading < 0.0f)
        heading += 360.0f;

    return heading;
}
