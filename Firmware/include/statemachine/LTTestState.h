#pragma once


#include "State.h"
#include "stdint.h"

#include "Services.h"   
#include "Player.h"

class WeaponManager;

class LTTestState : public State
{
public:
       
    LTTestState(CombatLRServices& services, WeaponManager& weaponManager);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:   

    WeaponManager& _weaponManager;

    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  

    static const int HealthX = 0;
    static const int HealthY = 225;
    static const int HealthW = 40 ;
    static const int HealthH = 15;

    CombatLRServices& _services;
    
    Player _player;


    static const int ShotHitLedTime = 500;
    static const int DefaultStartHealth = 5;
    static const int ShotPower = 1;    // 5 hits before dead
    static const uint16_t DefaultAmmo = 999;

    static const int TimeToClearLTData = 1000;
    static const uint8_t DefaultWeaponIndex = 0;
    static const int InterShotTime = 100;
    static const size_t LTDataTextSize = 16;

};