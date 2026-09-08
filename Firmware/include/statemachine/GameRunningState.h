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

class GameRunningState : public State
{
public:
       
    GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    static const int PlayerStartHealth = 50;
    uint16_t    _gameTime;
    CombatLRServices& _stateData;
    WeaponManager& _weaponManager;
    Player  _player;
    AmmoPickup _ammoPickup;
    WeaponPickup _weaponPickup;
    JsonWeaponPickupTable _weaponPickupTable;
                
    double _startTime;
    void CheckPickups();

    void PickupBonus(ZoneEngine::PickupInfo &pickup);
    void PickupHealth(ZoneEngine::PickupInfo &pickup);
    void PickupAmmo(ZoneEngine::PickupInfo &pickup);

    static const int HealthX = 0;
    static const int HealthY = 225;
    static const int HealthW = 40 ;
    static const int HealthH = 15;

};
