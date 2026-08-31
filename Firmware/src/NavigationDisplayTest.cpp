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
#include "widgets/ZoneWidget.h"
#include "IDisplay.h"
#include "TextureManager.h"
#include "widgets/MapWidget.h"
#include <LittleFS.h>
#include "Ports.h"
#include "SteppedZoom.h"
#include "ZoomCommand.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "DisplayManager.h"
#include "ConfigureMapCommand.h"
#include "AddWidgetCommand.h"
#include "AddZoneCommand.h"
#include "PickupTypes.h"
#include "ZoneEngine.h"
#include "statemachine/PowerUpState.h"
#include "MAX98357ASounds.h"
#include "statemachine/CombatLRStateMachine.h"



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
 
    IZoom* zoomDevice = nullptr;

    TextureManager textureManager(LittleFS);
    DisplayManager displayManager(textureManager);

    ZoneEngine zoneEngine(gamePlan, displayManager);
    MAX98357ASounds sounds(LittleFS,BCLK,LRC,DATA);
    NavigationData data;
    Pacer zoneUpdatePacer(true);
    uint32_t gameTimeInSec=0;
    CombatLRStateMachine stateMachine(displayManager, sounds, zoneEngine, navigation);

};


  void UpdateNavigationDisplay()
{  
     stateMachine.Run();
    return;
   
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
                    
               
        displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new PlayerPositionCommand(
                    playerPosition)));

        displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new TurnMapToHeadingCommand(
                    data.heading)));


        std::vector<ZoneEngine::PickupInfo> pickups;
       if(zoneEngine.GetPickupsWithin(playerPosition, 4.0f, pickups))
       {
            for(int i = 0 ; i < pickups.size() ; i++)
            {
                Serial.printf("PickId: %d PickType :%d\n",pickups[i].id, pickups[i].type);
            }
       }
                
    }       
    
    float zoom = 1.0f;
    if(zoomDevice->GetZoom(zoom))
    {
        
        displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new ZoomCommand(
                zoom)));    
    }

    if(zoneUpdatePacer.Pace())
    {
        gameTimeInSec++;

        
    }

    zoneEngine.Update(gameTimeInSec);

}

void NavigationDisplayTestBegin()
{
    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("Navigation display integration test"));
    Serial.println(F("===================================="));

    displayManager.Begin();
    sounds.Begin();
   
    if(!gamePlan.Load("/gameplan.json"))
    {
        Serial.println("Failed to load gameplan");
        while(1)
        {};
    }
    zoneUpdatePacer.SetPace(1000);

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

    zoomDevice = new SteppedZoom(ZOOMSWITCHPORT);

    /*
    const Zone& z = gamePlan.GetZones()[0];
    AddZoneCommand* addZone = new AddZoneCommand(z.id, GameCoordinate(z.eastMetres, z.northMetres), z.radiusMetres, TFT_GREEN);
    displayManager.Send(
            std::unique_ptr<IDisplayCommand>(addZone));

    if(gamePlan.GetZones()[0].pickups.size())
    {
        std::map<int ,  Pickup>::const_iterator pickupIter = gamePlan.GetZones()[0].pickups.find(3);
        while(pickupIter != gamePlan.GetZones()[0].pickups.cend())
        {
            GameCoordinate position = pickupIter->second.GetPosition(); 

            std::string filename;
            switch(pickupIter->second.type)
            {
                case PickupTypes::Health:
                {
                    filename = "/health.bmp";
                }
                break;
                case PickupTypes::Ammo:
                {
                    filename = "/reload.bmp";
                }
                break;
                
                case PickupTypes::Bonus:
                {
                    filename = "/weapon.bmp";
                }
                break;
            }

            AddWidgetCommand* command = new AddWidgetCommand(pickupIter->second.id, WidgetType::Pickup, position, filename);
            Serial.print("Adding widget type "); Serial.println(pickupIter->second.type);
            displayManager.Send(
            std::unique_ptr<IDisplayCommand>(command));
            pickupIter++;
        }
         
    }
        */
    stateMachine.Init();
    stateMachine.Start();

}


