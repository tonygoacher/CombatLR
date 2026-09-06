
#include "weapons/WeaponPickup.h"

#include "weapons/WeaponManager.h"
#include "ZoneEngine.h"
#include "weapons/IWeaponPickupTable.h"

#include <algorithm>
#include <vector>

WeaponPickup::WeaponPickup(
    WeaponManager& weaponManager,
    ZoneEngine& zoneEngine,
    IWeaponPickupTable& pickupTable)
    :
    _weaponManager(weaponManager),
    _zoneEngine(zoneEngine),
    _pickupTable(pickupTable)
{
    Serial.println("Weapons pickup");
}

void WeaponPickup::Collect()
{
    /*
     * ZoneManager returns progression as 0.0 - 1.0.
     * The pickup table uses 0 - 100.
     */
    const float progress =
        std::clamp(
            _zoneEngine.GetProgress(),
            0.0f,
            1.0f);

    const uint8_t gameProgress =
        static_cast<uint8_t>(
            progress * 100.0f);

    const WeaponType type =
        SelectWeaponType(gameProgress);

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
    Serial.printf("Weapon pickup adding new weapoin %d\n", type);
    _weaponManager.AddWeapon(type);
}

WeaponType WeaponPickup::SelectWeaponType(
    uint8_t gameProgress) const
{
    const WeaponManager::WeaponMap& weapons =
        _weaponManager.GetWeaponMap();

    uint16_t totalChance = 0;

    /*
     * Build the weighted pool from all weapon types.
     *
     * The pickup table returns zero for weapons which
     * are not yet available at the current progression.
     */
    std::vector<WeaponType> availableTypes;
    std::vector<uint8_t> chances;

    for (int typeValue = 0;
         typeValue <=
             static_cast<int>(WeaponType::AssaultRifleFullAuto);
         ++typeValue)
    {
        const WeaponType type =
            static_cast<WeaponType>(typeValue);

        const uint8_t chance =
            _pickupTable.GetPickupChance(
                type,
                gameProgress);

        if (chance == 0)
            continue;

        availableTypes.push_back(type);
        chances.push_back(chance);

        totalChance += chance;
    }

    if (availableTypes.empty())
    {
        /*
         * No weapon is available according to the
         * pickup table. This should normally indicate
         * a configuration error.
         */
        return WeaponType::AssaultRifle;
    }

    const uint16_t roll =
        random(0, totalChance);

    uint16_t accumulated = 0;

    for (size_t i = 0;
         i < availableTypes.size();
         ++i)
    {
        accumulated += chances[i];

        if (roll < accumulated)
            return availableTypes[i];
    }

    return availableTypes.back();
}
