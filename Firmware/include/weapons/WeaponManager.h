#pragma once

#include <stdint.h>
#include <map>




class PBSwitch;
class IRLaserTag;
class DisplayManager;
class Texture;

enum class WeaponType
{
    AssaultRifle,
};

class Weapon
{
public:
    Weapon(WeaponType type, uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger)
    : _type(type), _laserTag(laserTag), _reserveAmmo(ammo), _magCapacity(magSize), _trigger(trigger)
    {
        CompleteReload();
    };

    virtual Texture& GetTexture() = 0;
    uint16_t GetRemainingAmmo(){ return _reserveAmmo;}
    uint16_t GetMagAmmo() {return _magAmmo;}

    virtual void Reload()=0;
    virtual void Run()=0;
    WeaponType GetWeaponType();
    virtual void AddAmmo(uint16_t addAmmo);

    void Weapon::CompleteReload()       // This is the final act of moving the ammo into the mag
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
    WeaponType  _type;
    uint16_t    _reserveAmmo;
    uint16_t    _magAmmo;
    uint16_t    _magCapacity;    
    PBSwitch&   _trigger;
    IRLaserTag&    _laserTag;
};

class WeaponManager
{
public:
    WeaponManager& Instance();
    void Init(IRLaserTag& laserTag, PBSwitch& trigger, DisplayManager& displayManager);

    void Run(); // Used to pump the current weapon
    void AddWeapon(WeaponType type, uint16_t initialAmmo);
    void AddAmmo(WeaponType type, uint16_t ammo);

private:
    std::map<WeaponType, Weapon&> _weapons;
    IRLaserTag& _laserTag;
    PBSwitch& _trigger;
    DisplayManager& _display;
};