#include "NavigationDisplayTest.h"

#include <Arduino.h>

#include "SpoofNavigation.h"
#include "GeoReference.h"
#include "NavigationNMEA.h"
#include "GamePlan.h"
#include "MapReference.h"
#include "MillisGameTime.h"
#include "GameController.h"
#include "ZoneWidget.h"
#include "IDisplay.h"

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
    MillisGameTime gameTime;
    

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
};

  NavigationData data;
  void UpdateNavigationDisplay()
{
    Serial.println("UpdateNavigationDisplay");

  
    if(navigation.Update())
    {
    
        data = navigation.GetData();
    }

    if(!data.positionValid)
    {
        return;
    }

    constexpr double TEST_LATITUDE =
        53.5506262;

    constexpr double TEST_LONGITUDE =
        -1.9986439;



    /*
     * Convert the known C GPS position into the local
     * GameCoordinate system.
     */
    const GameCoordinate position =
        geoReference.ToGameCoordinate(
            data.latitude,
            data.longitude);
            

    Serial.printf(
        "TEST C: GPS=(%.7f, %.7f)\n",
        TEST_LATITUDE,
        TEST_LONGITUDE);

    Serial.printf(
        "TEST C: GameCoordinate east=%+.3fm north=%+.3fm\n",
        position.east,
        position.north);

    /*
     * Convert the same position into the BMP coordinate system.
     */
    float mapX;
    float mapY;

    if (mapReference.GameToMapPixel(
            position,
            mapX,
            mapY))
    {
        Serial.printf(
            "TEST C: Map=(%.2f, %.2f) px\n",
            mapX,
            mapY);
    }
    else
    {
        Serial.println(
            "TEST C: GameToMapPixel FAILED");
    }


    

    /*
     * Make this known position the player position.
     */
    SetNavigationPlayerPosition(
        position,
        data.heading);
}

void NavigationDisplayTestBegin(IDisplay* display)
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

  
    ZoneWidget* z = new ZoneWidget(gamePlan.GetZones()[0]);
    display->AddWidget(z);

    GameController::Instance().Initialise(gamePlan, gameTime);

    GameController::Instance().ShowZoneInfo();

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

