#pragma once

#include "Navigation.h"
#include "SpoofNavigation.h"
#include "GeoReference.h"

/*
 * Small helper for exercising the complete navigation -> world-coordinate
 * pipeline before real GPS hardware is connected.
 */
class NavigationTest
{
public:
    NavigationTest();

    void Begin(
        double homeLatitude,
        double homeLongitude);

    void SetPlayerRelative(
        float eastMeters,
        float northMeters,
        float headingDegrees = 0.0f);

    void Update();

    GameCoordinate GetPlayerCoordinate() const;
    NavigationData GetNavigationData() const;

private:
    SpoofNavigation _navigation;
    GeoReference _geoReference;
    GameCoordinate _playerCoordinate;
};
