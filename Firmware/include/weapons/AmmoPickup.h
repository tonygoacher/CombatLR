
#pragma once

#include "weapons/Weapon.h"

class WeaponManager;

class AmmoPickup
{
public:
    explicit AmmoPickup(WeaponManager& weaponManager);

    void Collect();

private:
    WeaponManager& _weaponManager;

    static constexpr uint8_t ReloadPercentage = 30;

    // For now , any weapon at or below this ID is guarteed a reload weapon pickup. This probably should be in gameplan.
    static constexpr WeaponType GuaranteedReloadType = 3;

};

