
#include "weapons/AmmoPickup.h"

#include "weapons/WeaponManager.h"

#include <algorithm>
#include <vector>

AmmoPickup::AmmoPickup(WeaponManager& weaponManager)
    :
    _weaponManager(weaponManager)
{
    Serial.println("Ammo pickup");
}

void AmmoPickup::Collect()
{
    const WeaponManager::WeaponMap& weapons =
        _weaponManager.GetWeaponMap();

    if (weapons.empty())
        return;

    const size_t weaponCount = weapons.size();

    // Select approximately 30% of the current loadout.
    // Always select at least one weapon.
    size_t reloadCount =
        (weaponCount * ReloadPercentage + 99) / 100;

    reloadCount =
        std::max<size_t>(1, reloadCount);

    reloadCount =
        std::min(reloadCount, weaponCount);

    std::vector<WeaponType> selected;
    selected.reserve(reloadCount);

    /*
     * Guarantee one weapon at or below the
     * AssaultRifle power level.
     */
    for (const auto& entry : weapons)
    {
        if (static_cast<int>(entry.first) <=
            static_cast<int>(GuaranteedReloadType))
        {
            selected.push_back(entry.first);
            break;
        }
    }

    /*
     * Build a list of weapons which have not
     * already been selected.
     */
    std::vector<WeaponType> available;
    available.reserve(weaponCount);

    for (const auto& entry : weapons)
    {
        if (std::find(
                selected.begin(),
                selected.end(),
                entry.first) == selected.end())
        {
            available.push_back(entry.first);
        }
    }

    /*
     * Randomly select the remaining weapons.
     */
    while (selected.size() < reloadCount &&
           !available.empty())
    {
        const size_t index =
            random(0, available.size());

        selected.push_back(available[index]);

        available.erase(
            available.begin() + index);
    }

        
    /*
    * Apply the reload to each selected weapon.
    */
    for (const WeaponType type : selected)
    {
        const auto weaponIt =
            weapons.find(type);

        if (weaponIt == weapons.end())
            continue;

        Weapon& weapon =
            *weaponIt->second;

        const uint16_t currentReserve =
            weapon.GetRemainingAmmo();

        const uint16_t maxReserve =
            weapon.GetMaxAmmoReserve();

        if (currentReserve >= maxReserve)
            continue;

        const uint16_t maxReload =
            weapon.GetMaxReload();

        const uint16_t minReload =
            (maxReload * 30) / 100;

        const uint16_t reloadAmount =
            random(minReload, maxReload + 1);

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
    }

}
