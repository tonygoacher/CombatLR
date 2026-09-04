#pragma once

#include "weapons/Weapon.h"
#include <stdint.h>
#include <map>
#include "stateMachine/Services.h"





class PBSwitch;
class IRLaserTag;
class DisplayManager;
class Texture;


class WeaponManager
{
public:
    WeaponManager(CombatLRServices& stateData);
    void Init();
    void Reset();
    void Run(); // Used to pump the current weapon
    void AddWeapon(WeaponType type, uint16_t initialAmmo);
    void AddAmmo(WeaponType type, uint16_t ammo);

private:

    enum class WMStates
    {
        Normal,
        Select,
    };

    std::map<WeaponType, Weapon&> _weapons;
    std::map<WeaponType, Weapon&>::iterator _currentWeapon;
    std::map<WeaponType, Weapon&>::iterator _displayWeapon;

    CombatLRServices& _stateData;
    WMStates    _state;

    void ShowSelectWeapon();
    void ShowLoadout (std::map<WeaponType, Weapon&>::iterator& weapon);


    static const int TextX = 280;
    static const int TextY=10;
    static const int TextZoom = 2;
    static const int TextFont = 2; 




};
