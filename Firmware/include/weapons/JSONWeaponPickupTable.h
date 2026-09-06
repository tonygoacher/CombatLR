
#pragma once

#include "IWeaponPickupTable.h"

#include <stdint.h>
#include <vector>

class JsonWeaponPickupTable : public IWeaponPickupTable
{
public:
    explicit JsonWeaponPickupTable(const char* filename);

    uint8_t GetPickupChance(
        WeaponType type,
        uint8_t gameProgress) const override;

private:
    struct WeaponPickupDefinition
    {
        WeaponType type;
        uint8_t pickupChance;
        uint8_t minimumProgress;
    };

    std::vector<WeaponPickupDefinition> _weapons;

    bool Load(const char* filename);
};
