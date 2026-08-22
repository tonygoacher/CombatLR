#include "NavigationDisplayTest.h"

#include <Arduino.h>

#include "SpoofNavigation.h"
#include "GeoReference.h"
#include "NavigationNMEA.h"
#include "GamePlan.h"
#include "MapReference.h"

/*
 * Production navigation/display API.
 *
 * These are free functions implemented by ST7789VDisplay.cpp.
 */
void ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude);

void SetNavigationPlayerPosition(
    const GameCoordinate& position,
    float headingDegrees);

namespace
{
    //SpoofNavigation navigation;
    NavigationNMEA  navigation(9,46);
    GeoReference geoReference;
    MapReference mapReference;

    constexpr double HOME_LATITUDE  = 53.593000;
    constexpr double HOME_LONGITUDE = -2.123000;

    constexpr float TEST_MAP_UP_HEADING = 77.0f;

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

    GamePlan gamePlan;

   void UpdateNavigationDisplay()
{
    Serial.println("UpdateNavigationDisplay");

    /*
     * Continue receiving NMEA so we know the receiver is working,
     * but use the test position for the display.
     */
 //   if (!navigation.Update())
    {
   //     Serial.println("No update available");
     //   return;
    }

    const NavigationData data =
        navigation.GetData();

    /*
     * Simulator position is authoritative for this display test.
     */

    /*
     * GPS position is authoritative for the display.
     *
     * The GPS position is converted into the same local
     * GameCoordinate system used by MapReference.
     */
  constexpr double TEST_LATITUDE  = 53.5512359;
constexpr double TEST_LONGITUDE = -1.9986676;

GameCoordinate position =
    geoReference.ToGameCoordinate(
        TEST_LATITUDE,
        TEST_LONGITUDE);

    SetNavigationPlayerPosition(
        position,
        TEST_MAP_UP_HEADING);

    /*
     * The simulator position is printed separately.
     * This is the position we expect the GPS to represent.
     */
    Serial.printf(
        "Simulator Player: east=%+.1fm north=%+.1fm heading=%+.1f\n",
        testEast,
        testNorth,
        TEST_MAP_UP_HEADING);

    /*
     * This is the actual position received from the GPS/NMEA data.
     */
    Serial.printf(
        "GPS Player: east=%+.1fm north=%+.1fm heading=%+.1f\n",
        position.east,
        position.north,
        data.heading);

    /*
     * Show where the actual GPS position maps onto the source map.
     */
    float mapX;
    float mapY;

    if (mapReference.GameToMapPixel(
            position,
            mapX,
            mapY))
    {
        Serial.printf(
            "GPS Map: x=%.2f y=%.2f px\n",
            mapX,
            mapY);
    }

    /*
 * C is the known reference point on the BMP.
 * Its GPS position is the same as TEST_LATITUDE/TEST_LONGITUDE.
 */
GameCoordinate cPosition =
    geoReference.ToGameCoordinate(
        TEST_LATITUDE,
        TEST_LONGITUDE);

float cMapX;
float cMapY;

if (mapReference.GameToMapPixel(
        cPosition,
        cMapX,
        cMapY))
{
    Serial.printf(
        "C Map: x=%.2f y=%.2f px\n",
        cMapX,
        cMapY);
}
}
}

void NavigationDisplayTestBegin()
{
    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("Navigation display integration test"));
    Serial.println(F("===================================="));

    if(!gamePlan.Load("/gameplan.json"))
    {
        Serial.println("Failed to load gameplan");
        while(1)
        {};
    }
    else
    {
        Serial.println("Gameplan loaded ok");
    }

    if (!mapReference.Load("/map.json"))
    {
        Serial.println("Map reference load failed");
        while(1)
        {};
        // fa
    }    

    ConfigureNavigationMap(
    "/map.json",
    gamePlan.GetHomeLatitude(),
    gamePlan.GetHomeLongitude());

    Serial.print("Home lat "); Serial.println(gamePlan.GetHomeLatitude());
    Serial.print("Home lon "); Serial.println(gamePlan.GetHomeLongitude());

    geoReference.SetHome(
    gamePlan.GetHomeLatitude(),
    gamePlan.GetHomeLongitude());

mapReference.Configure(
    gamePlan.GetHomeLatitude(),
    gamePlan.GetHomeLongitude());

    GameCoordinate home;
home.east = 0.0f;
home.north = 0.0f;

float homeX;
float homeY;

if (mapReference.GameToMapPixel(home, homeX, homeY))
{
    Serial.printf(
        "Map Home: x=%.2f y=%.2f px\n",
        homeX,
        homeY);
}


    navigation.Begin();


    UpdateNavigationDisplay();

    lastMove = millis();
}

void NavigationDisplayTestLoop()
{
    if (millis() - lastMove < MOVE_INTERVAL_MS)
        return;

    lastMove = millis();

 


    UpdateNavigationDisplay();
}
