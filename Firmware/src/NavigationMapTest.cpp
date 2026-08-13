/*
 * Navigation / map coordinate test
 *
 * This test does not require real GPS hardware.
 * It exercises:
 *
 *   SpoofNavigation
 *        -> NavigationData
 *        -> GeoReference
 *        -> GameCoordinate
 *        -> MapCoordinate
 *
 * It is intended to be called from the existing Arduino setup()/loop().
 */

#include <Arduino.h>

#include "NavigationTest.h"
#include "MapCoordinate.h"

namespace
{
    NavigationTest navigationTest;
    MapCoordinate mapCoordinate;

    // Example Home position.
    constexpr double HOME_LATITUDE  = 53.593000;
    constexpr double HOME_LONGITUDE = -2.123000;

    // Example map calibration.
    // Replace these with the actual Home pixel and map scale later.
    constexpr float MAP_HOME_X = 640.0f;
    constexpr float MAP_HOME_Y = 480.0f;
    constexpr float PIXELS_PER_METRE = 2.0f;

    uint32_t lastTest = 0;
    uint8_t testPosition = 0;

    struct TestPoint
    {
        float east;
        float north;
        float heading;
    };

    constexpr TestPoint TEST_POINTS[] =
    {
        {   0.0f,    0.0f,   0.0f },   // Home
        {  50.0f,    0.0f,  90.0f },   // 50m east
        {  50.0f, 100.0f,  90.0f },   // 50m east, 100m north
        {   0.0f,  100.0f, 180.0f },   // 100m north
        {-100.0f,  100.0f, 270.0f },   // 100m west, 100m north
        {-100.0f,    0.0f,   0.0f },   // 100m west
        {   0.0f,    0.0f,   0.0f }    // Back Home
    };

    constexpr size_t TEST_POINT_COUNT =
        sizeof(TEST_POINTS) / sizeof(TEST_POINTS[0]);

    void PrintCurrentPosition()
    {
        const NavigationData navigation =
            navigationTest.GetNavigationData();

        const GameCoordinate game =
            navigationTest.GetPlayerCoordinate();

        float mapX;
        float mapY;

        mapCoordinate.ToMapPixel(
            game,
            mapX,
            mapY);

        Serial.println();
        Serial.println(F("---- Navigation test ----"));

        Serial.printf(
            "GPS: %.8f, %.8f\n",
            navigation.latitude,
            navigation.longitude);

        Serial.printf(
            "Heading: %.1f deg\n",
            navigation.heading);

        Serial.printf(
            "Game: east=%+.2f m, north=%+.2f m\n",
            game.east,
            game.north);

        Serial.printf(
            "Map:  x=%.2f, y=%.2f px\n",
            mapX,
            mapY);
    }
}

void NavigationMapTestBegin()
{
    Serial.println();
    Serial.println(F("================================"));
    Serial.println(F("Navigation / Map coordinate test"));
    Serial.println(F("================================"));

    navigationTest.Begin(
        HOME_LATITUDE,
        HOME_LONGITUDE);

    mapCoordinate.Configure(
        MAP_HOME_X,
        MAP_HOME_Y,
        PIXELS_PER_METRE);

    Serial.printf(
        "Home: %.8f, %.8f\n",
        HOME_LATITUDE,
        HOME_LONGITUDE);

    Serial.printf(
        "Map Home: %.1f, %.1f\n",
        MAP_HOME_X,
        MAP_HOME_Y);

    Serial.printf(
        "Map scale: %.2f pixels/metre\n",
        PIXELS_PER_METRE);

    PrintCurrentPosition();

    lastTest = millis();
}

void NavigationMapTestLoop()
{
    if (millis() - lastTest < 2000)
        return;

    lastTest = millis();

    const TestPoint& point =
        TEST_POINTS[testPosition];

    Serial.printf(
        "\nMoving spoof player to %+0.1fm east, %+0.1fm north\n",
        point.east,
        point.north);

    navigationTest.SetPlayerRelative(
        point.east,
        point.north,
        point.heading);

    PrintCurrentPosition();

    ++testPosition;

    if (testPosition >= TEST_POINT_COUNT)
    {
        testPosition = 0;

        Serial.println();
        Serial.println(F("*** Test path complete ***"));
    }
}
