
#pragma once

#include "weapons/Weapon.h"

class WeaponManager;
class ZoneEngine;
class IWeaponPickupTable;

class WeaponPickup
{
public:
    WeaponPickup(
        WeaponManager& weaponManager,
        ZoneEngine& zoneEngine,
        IWeaponPickupTable& pickupTable);

    void Collect();

private:
    WeaponManager& _weaponManager;
    ZoneEngine& _zoneEngine;
    IWeaponPickupTable& _pickupTable;

    static constexpr uint8_t MinPickupPercentage = 30;

    WeaponType SelectWeaponType(
        uint8_t gameProgress) const;
};
