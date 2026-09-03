#pragma once

#include "weapons/Weapon.h"
#include <stdint.h>
#include <map>




class PBSwitch;
class IRLaserTag;
class DisplayManager;
class Texture;


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