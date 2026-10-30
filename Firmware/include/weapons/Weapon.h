#pragma once

#include <stdint.h>
#include "graphics/Texture.h"

class PBSwitch;
class IRLaserTag;
class DisplayManager;
class ISounds;
class CombatLRServices;

typedef int   WeaponType;

class WeaponConfig;




class Weapon
{
public:
    Weapon(  WeaponType type,  CombatLRServices& services);

    virtual ~Weapon() = default;

    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}  // Total remaining ammo minu mag
    uint16_t GetMagAmmo() {return _magAmmo;}            // Rounds currently in the mag
    uint16_t GetDefaultAmmoCount();     // Returns the default ammo for a newly picked up weapon
    uint16_t GetMaxReload();          // Return the max ammo for a given reload
    uint16_t GetMagCapacity();              // Return the max rounds in a mag
    uint16_t GetMaxAmmoReserve();

    virtual void AddAmmo(uint16_t addAmmo) = 0; // Called when a pick up is made




    WeaponType GetWeaponType() { return _type;}

    virtual void Equip() = 0;    // Called when the when is activated (slung)
    virtual void Unslung() = 0;    // Called when the weapon is deativated (unslung)
    void CompleteReload();       // This is the final act of moving the ammo into the mag
    void TakeShot();
    
    virtual const char * const GetFilename() = 0;
protected:
    static const int MaxFileName =128;

    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo; 
    CombatLRServices& _services;
    Texture*     _texture;
    WeaponType  _type;
};
