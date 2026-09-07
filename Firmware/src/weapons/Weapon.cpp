
#include "weapons/Weapon.h"
#include "weapons/WeaponConfig.h"
#include "graphics/TextureManager.h"
#include "countof.h"


Weapon::Weapon(WeaponType type, WeaponConfig& weaponConfig, CombatLRServices& services)
:    _services(services), _magAmmo(0), _type(type), _weaponConfig(weaponConfig)
{
    _reserveAmmo = GetDefaultAmmoCount();
    CompleteReload();
};





uint16_t Weapon::GetMaxReload()
{
    return _weaponConfig.GetMaxReload(_type);
}



uint16_t Weapon::GetDefaultAmmoCount()
{
    return _weaponConfig.GetDefaultAmmoCount(_type);
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


uint16_t Weapon::GetMaxAmmoReserve()
{
    return _weaponConfig.GetMaxAmmoReserve(_type);
}



uint16_t Weapon::GetMagCapacity()
{
    return _weaponConfig.GetMagCapacity(_type);
}
