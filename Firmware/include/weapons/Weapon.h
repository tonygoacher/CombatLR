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
    Weapon( uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds);

    ~Weapon() = default;

    Texture& GetTexture();
    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}
    uint16_t GetMagAmmo() {return _magAmmo;}

    virtual void Run()=0;
    virtual WeaponType GetWeaponType() =0;
    virtual void AddAmmo(uint16_t addAmmo) = 0;
    virtual void Start() = 0;   // Called when the when is activated (slung)
    virtual void Stop() = 0;    // Called when the weapon is deativated (unslung)

    uint16_t  GetReserve() { return _reserveAmmo;}
    uint16_t  GetMax() { return _magAmmo;}
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


protected:
    static const int MaxFileName =128;
    virtual const char * const GetFilename() = 0;
    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo;
    uint16_t    _magCapacity;    
    PBSwitch&   _trigger;
    PBSwitch&   _reload;
    IRLaserTag&  _laserTag;
    Texture     _texture;
    ISounds&     _sounds;
};