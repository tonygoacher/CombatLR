#pragma once

#include "weapons/Weapon.h"

class WeaponManager;

class WeaponPickup
{
public:
    explicit WeaponPickup(WeaponManager& weaponManager);

    void Collect();

private:
    WeaponManager& _weaponManager;

    static constexpr uint8_t MinPickupPercentage = 30;

    WeaponType SelectWeaponType() const;
};
