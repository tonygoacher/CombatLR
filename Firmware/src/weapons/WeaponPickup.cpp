#include "weapons/WeaponPickup.h"

#include "weapons/WeaponManager.h"
#include "ZoneEngine.h"
#include "weapons/IWeaponPickupTable.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

WeaponPickup::WeaponPickup(
    WeaponManager& weaponManager,
    ZoneEngine&,
    IWeaponPickupTable&)
    : _weaponManager(weaponManager)
{
    Serial.println("Weapons pickup");
}

void WeaponPickup::Collect()
{
    const WeaponType type = SelectWeaponType();

    const WeaponManager::WeaponMap& weapons =
        _weaponManager.GetWeaponMap();

    const auto weaponIt =
        weapons.find(type);

    /*
     * Weapon already exists.
     *
     * Treat the pickup as ammunition for that weapon.
     */
    if (weaponIt != weapons.end())
    {
        Serial.printf("Weapon type %d already exists \n", static_cast<int>(weaponIt->first));
        Weapon& weapon =
            *weaponIt->second;

        const uint16_t currentReserve =
            weapon.GetRemainingAmmo();

        const uint16_t maxReserve =
            weapon.GetMaxAmmoReserve();

        if (currentReserve >= maxReserve)
            return;

        const uint16_t maxReload =
            weapon.GetMaxReload();

        if (maxReload == 0)
            return;

        const uint16_t minReload =
            std::max<uint16_t>(
                1,
                (maxReload * MinPickupPercentage) / 100);

        const uint16_t reloadAmount =
            random(
                minReload,
                maxReload + 1);

        const uint16_t availableSpace =
            maxReserve - currentReserve;

        const uint16_t actualReload =
            std::min(
                reloadAmount,
                availableSpace);

        if (actualReload > 0)
        {
            _weaponManager.AddAmmo(
                type,
                actualReload);
        }

        return;
    }

    /*
     * New weapon.
     *
     * WeaponManager is responsible for creating the
     * weapon and establishing its initial ammunition.
     */
    Serial.printf("Weapon pickup adding new weapon %d\n", type);
    _weaponManager.AddWeapon(type);
}

WeaponType WeaponPickup::SelectWeaponType() const
{
    /*
     * Weapon definitions are ordered from least powerful
     * to most powerful in weapons.json. The type remains
     * an identifier; the position in that ordered list is
     * used only to calculate relative pickup weight.
     *
     * A moderate Gaussian curve reduces the probability
     * of the weakest and strongest entries without making
     * either impossible to obtain. No game-progress state
     * or separate pickup table is involved.
     */
    const int numWeapons = _weaponManager.GetNumWeapons();

    if (numWeapons <= 0)
    {
        Serial.println("No weapon definitions available for pickup");
        return 0;
    }

    std::vector<WeaponType> availableTypes;
    std::vector<uint32_t> weights;
    availableTypes.reserve(numWeapons);
    weights.reserve(numWeapons);

    uint32_t totalWeight = 0;
    const double midpoint = (numWeapons - 1) / 2.0;
    const double halfRange = midpoint > 0.0 ? midpoint : 1.0;

    for (int index = 0; index < numWeapons; ++index)
    {
        const double normalizedRank =
            (index - midpoint) / halfRange;

        // Fixed moderate curve; the parameters can become game-plan settings later.
        const double relativeWeight =
            std::exp(-0.75 * normalizedRank * normalizedRank);

        // Scale to integer weights for the weighted random selection.
        const uint32_t weight =
            static_cast<uint32_t>(relativeWeight * 1000.0 + 0.5);

        if (weight == 0)
            continue;

        availableTypes.push_back(static_cast<WeaponType>(index));
        weights.push_back(weight);
        totalWeight += weight;
    }

    if (availableTypes.empty() || totalWeight == 0)
    {
        Serial.println("No valid weapon types for pickup");
        return 0;
    }

    const uint32_t roll =
        static_cast<uint32_t>(random(0, static_cast<long>(totalWeight)));

    uint32_t accumulated = 0;

    for (size_t i = 0; i < availableTypes.size(); ++i)
    {
        accumulated += weights[i];

        if (roll < accumulated)
            return availableTypes[i];
    }

    return availableTypes.back();
}
