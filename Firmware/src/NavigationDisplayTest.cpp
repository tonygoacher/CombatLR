#include "NavigationDisplayTest.h"
#include "TFT_eSPI.h"

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
#include "TextureManager.h"
#include "MapWidget.h"
#include <SPIFFS.h>
#include "Ports.h"
#include "ADCZoom.h"
#include "ZoomCommand.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "DisplayManager.h"
#include "ConfigureMapCommand.h"


/*
 * Production navigation/display API.
 *
 * These are free functions implemented by ST7789VDisplay.cpp.
 */


void SetNavigationPlayerPosition(
    const GameCoordinate& position,
    float headingDegrees);

namespace
{
    //SpoofNavigation navigation;
    NavigationNMEA  navigation(9,46);
    GeoReference geoReference;
   
    MillisGameTime gameTime;
    IDisplay* theDisplay;

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
 
    IZoom* zoomDevice = new ADCZoom(ZOOMADC);
};

  NavigationData data;
  void UpdateNavigationDisplay(DisplayManager& displayManager)
{  
    if(navigation.Update())
    {
        
        data = navigation.GetData();

   

        /*
        * Convert the known C GPS position into the local
        * GameCoordinate system.
        */
        const GameCoordinate playerPosition =
            geoReference.ToGameCoordinate(
                data.latitude,
                data.longitude);

        Serial.printf("GameCoordinate Rx data:%.3f %.3f\n", playerPosition.east, playerPosition.north);
                    
        float zoom = zoomDevice->GetZoom();
                
        displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new PlayerPositionCommand(
                    playerPosition)));

        displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new TurnMapToHeadingCommand(
                    data.heading)));

        displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new ZoomCommand(
                    0.8f)));

                
    }                

}

void NavigationDisplayTestBegin(DisplayManager& displayManager)
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

    displayManager.ConfigureNavigationMap(
    "/map.json",
    gamePlan.GetHomeLatitude(),
    gamePlan.GetHomeLongitude());

    Serial.print("Home lat "); Serial.println(gamePlan.GetHomeLatitude());
    Serial.print("Home lon "); Serial.println(gamePlan.GetHomeLongitude());

    geoReference.SetHome(
    gamePlan.GetHomeLatitude(),
    gamePlan.GetHomeLongitude());


    GameController::Instance().Initialise(gamePlan, gameTime);

    GameController::Instance().ShowZoneInfo();

    navigation.Begin();
  
}


