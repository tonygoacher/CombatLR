#pragma once

#include <stdint.h>
#include "graphics/Texture.h"

class PBSwitch;
class IRLaserTag;
class DisplayManager;
class ISounds;
class CombatLRServices;

enum class WeaponType               // These must always start at 0 and be contiguous as the int value is used as an index
{
    AssaultRifle,
    AssaultRifleFullAuto,
    Blaster,
};

class WeaponConfig;




class Weapon
{
public:
    Weapon(  WeaponType type, WeaponConfig& weaponConfig,  CombatLRServices& services);

    virtual ~Weapon() = default;

    Texture& GetTexture();
    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}  // Total remaining ammo minu mag
    uint16_t GetMagAmmo() {return _magAmmo;}            // Rounds currently in the mag
    uint16_t GetDefaultAmmoCount();     // Returns the default ammo for a newly picked up weapon
    uint16_t GetMaxReload();          // Return the max ammo for a given reload
    uint16_t GetMagCapacity();              // Return the max rounds in a mag
    uint16_t GetMaxAmmoReserve();
  
    virtual bool Run()=0;
    virtual void AddAmmo(uint16_t addAmmo) = 0;
    virtual void Start() = 0;   // Called when the when is activated (slung)
    virtual void Stop() = 0;    // Called when the weapon is deativated (unslung)


    WeaponType GetWeaponType() { return _type;}

    virtual void Equip() = 0;
    void CompleteReload();       // This is the final act of moving the ammo into the mag
    

    virtual const char * const GetFilename() = 0;
protected:
    static const int MaxFileName =128;

    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo; 
    CombatLRServices& _services;
    Texture*     _texture;
    WeaponType  _type;
    WeaponConfig& _weaponConfig;
};
