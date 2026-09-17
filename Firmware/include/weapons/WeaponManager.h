#pragma once

#include "weapons/Weapon.h"
#include "weapons/WeaponConfig.h"
#include <stdint.h>
#include <map>
#include "stateMachine/Services.h"
#include "Pacer.h"





class PBSwitch;
class IRLaserTag;
class DisplayManager;
class Texture;


class WeaponManager
{



public:
    typedef std::map<WeaponType, std::shared_ptr<Weapon>> WeaponMap;

    WeaponManager(CombatLRServices& stateData);
    void Init();
    void Reset();
    void Run(); // Used to pump the current weapon
    void AddWeapon(WeaponType type);
    void AddAmmo(WeaponType type, uint16_t ammo);
    WeaponType GetCurrentWeaponType();
    int GetNumWeapons();
    const WeaponMap& GetWeaponMap();

private:


    enum class WMStates
    {
        Normal,
        Select,
    };



    typedef std::pair<WeaponType, std::shared_ptr<Weapon>> WeaponPair;
    WeaponMap _weapons;
    WeaponMap::iterator _currentWeapon;
    WeaponMap::iterator _displayWeapon;

    CombatLRServices& _stateData;
    WMStates    _state;
    int GetGuaranteedWeaponReload();


    void ShowSelectWeapon();
    void ShowLoadout (WeaponMap::iterator& weapon);


    static const int TextX = 270;
    static const int TextY=225;
    static const int TextZoom = 2;
    static const int TextFont = 2; 

    inline static constexpr uint16_t DefaultMagSize = 30;
     inline static constexpr uint16_t SelectTimeout = 4000; // Weapon select times out back to current weapons after this many ms

    Pacer _selectPacer;


    bool InsertWeapon(std::pair<WeaponMap::iterator , bool>& insertResult);


};

