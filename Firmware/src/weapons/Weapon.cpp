
#include "weapons/Weapon.h"
#include "TextureManager.h"
#include "countof.h"


Weapon::Weapon(WeaponType type, CombatLRServices& services)
:    _services(services), _magAmmo(0), _type(type)
{
    _reserveAmmo = GetDefaultAmmoCount();
    CompleteReload();
};





uint16_t Weapon::GetMaxReload()
{
    static const uint16_t maxReloads[] = 
    {
        300,        // SA80,
        300,        // M16
    };
    int index  = static_cast<int>(_type);
    if(index >= countof(maxReloads))
    {
        Serial.printf("GetMaxReload undefined value for weapon %d\n",index);
        return 0;
    }
    else
    {
        return maxReloads[index];
    }
}



uint16_t Weapon::GetDefaultAmmoCount()
{
    static const uint16_t defaultAmmo[] = 
    {
        120,        // SA80,
        120,        // M16
    };
    int index  = static_cast<int>(_type);
    if(index >= countof(defaultAmmo))
    {
        Serial.printf("GetDefaultAmmoCount undefined value for weapon %d\n",index);
        return 0;
    }
    else
    {
        return defaultAmmo[index];
    }
}



void Weapon::CompleteReload()   // Shift from remaining ammo into mag given mag capcity
{
    uint16_t required = GetMagCapacity() - _magAmmo;

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


uint16_t Weapon::GetMagCapacity()
{
    static const uint16_t magCapacity[] = 
    {
        30,        // SA80,
        30,        // M16
    };
    int index  = static_cast<int>(_type);
    if(index >= countof(magCapacity))
    {
        Serial.printf("GetMagCapacity undefined value for weapon %d\n",index);
        return 0;
    }
    else
    {
        return magCapacity[index];
    }
}
