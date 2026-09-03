#pragma once

#include <stdint.h>
#include "Texture.h"

class PBSwitch;
class IRLaserTag;
class DisplayManager;
class ISounds;

enum class WeaponType
{
    AssaultRifle,
};

class Weapon
{
public:
    Weapon( uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds)
    :  _laserTag(laserTag), _reserveAmmo(ammo), _magCapacity(magSize), _trigger(trigger), _reload(reload), _sounds(sounds)
    {
        CompleteReload();
    };

    ~Weapon() = default;

    Texture& GetTexture();
    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}
    uint16_t GetMagAmmo() {return _magAmmo;}

    virtual void Run()=0;
    virtual WeaponType GetWeaponType() =0;
    virtual void AddAmmo(uint16_t addAmmo) = 0;
    virtual void Start() = 0;   // Called when the when is activated (slung)
    virtual void Stop() = 0;    // Called when the weapon is deativated (unslung)

    void CompleteReload()       // This is the final act of moving the ammo into the mag
    {
        uint16_t required = _magCapacity - _magAmmo;

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
    }


protected:
    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo;
    uint16_t    _magCapacity;    
    PBSwitch&   _trigger;
    PBSwitch&   _reload;
    IRLaserTag&  _laserTag;
    Texture     _texture;
    ISounds&     _sounds;
};