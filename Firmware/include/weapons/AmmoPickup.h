
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

    // For now, AssaultRifle is the highest-power weapon
    // that qualifies for the guaranteed reload.
    static constexpr WeaponType GuaranteedReloadType =
        WeaponType::AssaultRifle;
};

