
#pragma once

#include "weapons/Weapon.h"

#include <stdint.h>

class IWeaponPickupTable
{
public:
    virtual ~IWeaponPickupTable() = default;

    /*
     * Returns the percentage chance of this weapon being
     * selected at the specified game progression.
     *
     * Returns 0 if the weapon is not currently available.
     *
     * gameProgress is 0 - 100.
     */
    virtual uint8_t GetPickupChance(
        WeaponType type,
        uint8_t gameProgress) const = 0;
};

