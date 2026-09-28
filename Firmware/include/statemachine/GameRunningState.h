#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"
#include "stateMachine/Services.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "widgets/TextWidget.h"
#include "stateMachine/Services.h"
#include "weapons/WeaponManager.h"
#include "ZoneEngine.h"
#include "Player.h"
#include "weapons/AmmoPickup.h"
#include "weapons/WeaponPickup.h"
#include "weapons/JSONWeaponPickupTable.h"
#include "IZoneEvent.h"
#include "IPickupEvent.h"
#include "SystemTime.h"

class GameRunningState : public State, public IZoneEvent,  public IPickupEvent
{
public:
       
    GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

    void ZoneMoveImminent(GameCoordinate newZonePos,int zoneIndexInGamePlan,uint32_t timeToActivation) override;
    void ZoneStartedMoving(GameCoordinate newZonePos, int zoneIndexInGamePlan) override;
    void NewZoneActive(GameCoordinate newZonePos,int zoneIndexInGamePlan) override;

    void PickupActivated(uint32_t pickupId) override;         // A timed pickup has been added to the map
    void PickupDeactivated(uint32_t pickupId) override;

private:    
    static const int PlayerStartHealth = 50;
    uint16_t    _gameTime;
    CombatLRServices& _stateData;
    WeaponManager& _weaponManager;
    Player  _player;
    AmmoPickup _ammoPickup;
    WeaponPickup _weaponPickup;
    JsonWeaponPickupTable _weaponPickupTable;
    SystemTime _systemTime;
                
    double _startTime;
    void CheckPickups();

    void PickupBonus(ZoneEngine::PickupInfo &pickup);
    void PickupHealth(ZoneEngine::PickupInfo &pickup);
    void PickupAmmo(ZoneEngine::PickupInfo &pickup);

    static const int HealthX = 0;
    static const int HealthY = 225;
    static const int HealthW = 40 ;
    static const int HealthH = 15;

    static const int ShotHitLedTime = 300;  // Time hit led is on when shot
    static const int ZoneHitLedTime = 1000; // Time hit LED is on when outside zone

};

