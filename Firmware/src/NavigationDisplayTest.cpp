#include "NavigationDisplayTest.h"

#include <Arduino.h>

#include "SpoofNavigation.h"
#include "GeoReference.h"

/*
 * Production navigation/display API.
 *
 * These are free functions implemented by ST7789VDisplay.cpp.
 */
void ConfigureNavigationMap(
    float mapHomeX,
    float mapHomeY,
    float pixelsPerMetre);

void SetNavigationPlayerPosition(
    const GameCoordinate& position,
    float headingDegrees);

namespace
{
    SpoofNavigation navigation;
    GeoReference geoReference;

    constexpr double HOME_LATITUDE  = 53.593000;
    constexpr double HOME_LONGITUDE = -2.123000;

    /*
     * Temporary calibration for the current test map.
     *
     * Home is at BMP pixel (640,480).
     * Scale is 2 pixels per metre.
     */
    constexpr float MAP_HOME_X = 640.0f;
    constexpr float MAP_HOME_Y = 480.0f;
    constexpr float PIXELS_PER_METRE = 2.0f;

    float testEast = 0.0f;
    float testNorth = 0.0f;
    float testHeading = 0.0f;

    uint32_t lastMove = 0;

    constexpr uint32_t MOVE_INTERVAL_MS = 1000;

    constexpr float EAST_STEP_METRES = 20.0f;
    constexpr float NORTH_STEP_METRES = 0.0f;
    constexpr float HEADING_STEP_DEGREES = 0.0f;// 15.0f;

    void UpdateNavigationDisplay()
    {
        const NavigationData data =
            navigation.GetData();

        if (!data.positionValid)
            return;

        const GameCoordinate position =
            geoReference.ToGameCoordinate(
                data.latitude,
                data.longitude);

        SetNavigationPlayerPosition(
            position,
            data.heading);

        Serial.printf(
            "Player: east=%+.1fm north=%+.1fm heading=%+.1f\n",
            position.east,
            position.north,
            data.heading);
    }
}

void NavigationDisplayTestBegin()
{
    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("Navigation display integration test"));
    Serial.println(F("===================================="));

    geoReference.SetHome(
        HOME_LATITUDE,
        HOME_LONGITUDE);

    ConfigureNavigationMap(
        MAP_HOME_X,
        MAP_HOME_Y,
        PIXELS_PER_METRE);

    navigation.Begin();

    testEast = 0.0f;
    testNorth = 0.0f;
    testHeading = 0.0f;

    navigation.SetPositionRelative(
        HOME_LATITUDE,
        HOME_LONGITUDE,
        testEast,
        testNorth);

    navigation.SetHeading(testHeading);

    UpdateNavigationDisplay();

    lastMove = millis();
}

void NavigationDisplayTestLoop()
{
    if (millis() - lastMove < MOVE_INTERVAL_MS)
        return;

    lastMove = millis();

    /*
     * Continuous test: no table and no index.
     */
    testEast += EAST_STEP_METRES;
    testNorth += NORTH_STEP_METRES;
    testHeading += HEADING_STEP_DEGREES;

    if (testHeading >= 360.0f)
        testHeading -= 360.0f;

    navigation.SetPositionRelative(
        HOME_LATITUDE,
        HOME_LONGITUDE,
        testEast,
        testNorth);

    navigation.SetHeading(testHeading);

    UpdateNavigationDisplay();
}
