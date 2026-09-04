#pragma once

#include <stdint.h>
#include "Texture.h"

class PBSwitch;
class IRLaserTag;
class DisplayManager;
class ISounds;
class CombatLRServices;

enum class WeaponType
{
    AssaultRifle,
    AssaultRifleFullAuto,
};

class Weapon
{
public:
    Weapon( uint16_t ammo, uint16_t magSize, CombatLRServices& services);

    virtual ~Weapon() = default;

    Texture& GetTexture();
    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}
    uint16_t GetMagAmmo() {return _magAmmo;}
  
    virtual bool Run()=0;
    virtual WeaponType GetWeaponType() = 0;
    virtual void AddAmmo(uint16_t addAmmo) = 0;
    virtual void Start() = 0;   // Called when the when is activated (slung)
    virtual void Stop() = 0;    // Called when the weapon is deativated (unslung)

    uint16_t  GetReserve() { return _reserveAmmo;}
    uint16_t  GetMax() { return _magAmmo;}
    virtual void Equip() = 0;
    void CompleteReload()       // This is the final act of moving the ammo into the mag
    {
        uint16_t required = _magCapacity - _magAmmo;

        Serial.println("Complete reload");
        Serial.printf("Res: %d Req: %d \n", _reserveAmmo, required);
        if (_reserveAmmo >= required)
        {
            _magAmmo += required;
            _reserveAmmo -= required;
        }
        else
        {
            _magAmmo += _reserveAmmo;
            _reserveAmmo = 0;
        }
        Serial.printf("Mag: %d\n", _magAmmo);
    }

    virtual const char * const GetFilename() = 0;
protected:
    static const int MaxFileName =128;

    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo;
    uint16_t    _magCapacity;    
    CombatLRServices& _services;
    Texture*     _texture;
};
