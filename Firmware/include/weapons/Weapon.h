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

    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}  // Total remaining ammo minus mag
    uint16_t GetReserveAndMagAmmo() { return _reserveAmmo + _magAmmo;}
    uint16_t GetMagAmmo() {return _magAmmo;}            // Rounds currently in the mag
    uint16_t GetDefaultAmmoCount();     // Returns the default ammo for a newly picked up weapon
    uint16_t GetMaxReload();          // Return the max ammo for a given reload
    uint16_t GetMagCapacity();              // Return the max rounds in a mag
    uint16_t GetMaxAmmoReserve();       // The max number of rounds that can be held in reserve
    uint16_t GetSpaceInMag() { return GetMagCapacity() - GetMagAmmo();} // Return how many rounds could be inserted into this mag

    virtual void AddAmmo(uint16_t addAmmo) = 0; // Called when a pick up is made
    virtual void SetAmmo(uint16_t ammo) = 0;




    WeaponType GetWeaponType() { return _type;}

    virtual void Equip() = 0;    // Called when the when is activated (slung)
    virtual void Unslung() = 0;    // Called when the weapon is deativated (unslung)
    void CompleteReload();       // This is the final act of moving the ammo into the mag
    void MoveToMag(uint16_t rounds);    // Move this many rounds from reserve to mag
    void TakeShot();
    
    virtual const char * const GetFilename() = 0;
protected:
    void PerformReload(uint16_t rounds);
    static const int MaxFileName =128;

    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo; 
    CombatLRServices& _services;
    Texture*     _texture;
    WeaponType  _type;
};
