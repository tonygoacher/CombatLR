#include "NavigationTest.h"

NavigationTest::NavigationTest()
    : _playerCoordinate{}
{
}

void NavigationTest::Begin(
    double homeLatitude,
    double homeLongitude)
{
    _geoReference.SetHome(
        homeLatitude,
        homeLongitude);

    _navigation.Begin();

    _navigation.SetPositionRelative(
        homeLatitude,
        homeLongitude,
        0.0f,
        0.0f);

    _navigation.SetHeading(0.0f);

    Update();
}

void NavigationTest::SetPlayerRelative(
    float eastMeters,
    float northMeters,
    float headingDegrees)
{
    _navigation.SetPositionRelative(
        _geoReference.GetHomeLatitude(),
        _geoReference.GetHomeLongitude(),
        eastMeters,
        northMeters);

    _navigation.SetHeading(headingDegrees);

    Update();
}

void NavigationTest::Update()
{
    _navigation.Update();

    const NavigationData data =
        _navigation.GetData();

    if (!data.positionValid ||
        !_geoReference.IsSet())
    {
        _playerCoordinate = {};
        return;
    }

    _playerCoordinate =
        _geoReference.ToGameCoordinate(
            data.latitude,
            data.longitude);
}

GameCoordinate NavigationTest::GetPlayerCoordinate() const
{
    return _playerCoordinate;
}

NavigationData NavigationTest::GetNavigationData() const
{
    return _navigation.GetData();
}
