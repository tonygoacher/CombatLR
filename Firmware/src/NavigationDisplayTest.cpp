#include "NavigationDisplayTest.h"
#include "TFT_eSPI.h"

#include <Arduino.h>


#include "GeoReference.h"
#include "NavigationNMEA.h"
#include "GamePlan.h"
#include "MapReference.h"
#include "MillisGameTime.h"
#include "GameController.h"
#include "widgets/ZoneWidget.h"
#include "graphics/IDisplay.h"
#include "graphics/TextureManager.h"
#include "widgets/MapWidget.h"
#include <LittleFS.h>
#include "Ports.h"
#include "SteppedZoom.h"
#include "graphics/ZoomCommand.h"
#include "graphics/PlayerPositionCommand.h"
#include "graphics/TurnMapToHeadingCommand.h"
#include "graphics/DisplayManager.h"
#include "graphics/ConfigureMapCommand.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/AddZoneCommand.h"
#include "PickupTypes.h"
#include "ZoneEngine.h"
#include "statemachine/PowerUpState.h"
#include "MAX98357ASounds.h"
#include "statemachine/CombatLRStateMachine.h"
#include "stateMachine/Services.h"
#include "IRLaserTag.h"
#include "weapons/WeaponManager.h"
#include "ESP32LaserTag.h"
#include "PacedLED.h"
#include "SystemConfig.h"
#include "SystemTime.h"




/*
 * Production navigation/display API.
 *
 * These are free functions implemented by ST7789VDisplay.cpp.
 */
class MockLaserTag : public IRLaserTag
{
public:
    void Init() override{};

    bool LaserTagDataAvailable() override { return false;}

    bool GetLasertagData(
        LaserTagData& data)  override { return false;}

    void SendLasertagData(
        LaserTagData& dataToSend) override {};
};

void SetNavigationPlayerPosition(
    const GameCoordinate& position,
    float headingDegrees);

namespace
{
    //SpoofNavigation navigation;
    NavigationNMEA  navigation(Ports::GPS_RX,Ports::GPS_TX,38400);
    GeoReference geoReference;
   
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
 
    TextureManager textureManager(LittleFS);
    DisplayManager displayManager(textureManager);

    ZoneEngine zoneEngine(gamePlan, displayManager);
    MAX98357ASounds sounds(LittleFS,BCLK,LRC,I2S_DATA);
    NavigationData data;
    Pacer zoneUpdatePacer(true);
    PBSwitch triggerSwitch(Ports::TRIGGER, false);
    PBSwitch reloadSwitch(Ports::RELOAD, true);
    PBSwitch leftSwitch(Ports::LEFT, true);
    PBSwitch zoomSwitch(Ports::ZOOM, true);
    PBSwitch rightSwitch(Ports::RIGHT, true);
    SteppedZoom zoom(zoomSwitch);
    PacedLED muzzleLED(MUZZLE_LED);
    PacedLED hitLED(HIT_LED);
    SystemTime systemTime(navigation);
    WeaponConfig weaponConfig;
    SystemConfig systemConfig;
    ESP32LaserTag laserTag(IR_TXPORT, IR_RXPORT, IR_POWERLOW, IR_POWERMED ,IR_POWERHIGH);
    CombatLRServices stateData{systemConfig,displayManager, navigation, 
                                zoneEngine, sounds, zoom,gamePlan, geoReference, laserTag, systemTime,
                                 weaponConfig, triggerSwitch, reloadSwitch, leftSwitch, zoomSwitch, rightSwitch,
                                 muzzleLED, hitLED};


    WeaponManager weaponManager(stateData);
    CombatLRStateMachine stateMachine(stateData,weaponManager);

};


  void UpdateNavigationDisplay()
{  

    muzzleLED.Run();
    hitLED.Run();
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
    

    zoneEngine.Update(systemTime.GetSeconds());

}

void NavigationDisplayTestBegin()
{
    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("Navigation display integration test"));
    Serial.println(F("===================================="));

    hitLED.Fire(3000);

    systemConfig.Begin();
   
    sounds.Begin();

    displayManager.Begin();
    weaponConfig.Init();

    sounds.SetVolume(1.0);
  
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



    navigation.Begin();


    stateMachine.Init();
    stateMachine.Start();

}


