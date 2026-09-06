#include "GamePlan.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace
{
    // Temporary allocation percentage. This will eventually come from
    // game configuration/JSON.
    constexpr uint8_t PickupAllocationPercentage = 40;

    struct PickupDistribution
    {
        uint8_t health;
        uint8_t ammo;
        uint8_t weapon;
    };

    // Temporary per-zone distribution table.
    // Indexed by zero-based zone number.
    //
    // These values are deliberately kept as a simple C++ table so that
    // the same structure can later be represented directly in JSON.
    constexpr PickupDistribution PickupDistributions[] =
    {
        { 40, 40, 20 },   // Zone 0
        { 40, 40, 20 },   // Zone 1
        { 40, 40, 20 },   // Zone 2
        { 40, 40, 20 },   // Zone 3
        { 40, 40, 20 },   // Zone 4
        { 40, 40, 20 },   // Zone 5
        { 40, 40, 20 },   // Zone 6
        { 40, 40, 20 },   // Zone 7
        { 40, 40, 20 },   // Zone 8
        { 40, 40, 20 }    // Zone 9
    };

    constexpr size_t PickupDistributionCount =
        sizeof(PickupDistributions) / sizeof(PickupDistributions[0]);

    constexpr PickupDistribution DefaultPickupDistribution = { 40, 40, 20 };

    double DistanceSquared(const Pickup& a, const Pickup& b)
    {
        const double dx = a.eastMetres - b.eastMetres;
        const double dy = a.northMetres - b.northMetres;

        return (dx * dx) + (dy * dy);
    }

    size_t CalculateSelectionCount(size_t candidateCount)
    {
        if (candidateCount == 0)
            return 0;

        const double selected =
            std::round(
                static_cast<double>(candidateCount) *
                static_cast<double>(PickupAllocationPercentage) /
                100.0);

        return std::max<size_t>(1, static_cast<size_t>(selected));
    }

    /*
     * Select a geographically well-spread subset of candidates.
     *
     * The first pickup is chosen randomly. Each subsequent pickup is the
     * candidate furthest from its nearest already-selected pickup.
     *
     * Selection is performed independently for each pickup type, so fixed
     * pickups do not act as anchors for Auto pickups.
     */
    std::vector<size_t> SelectEvenlyDistributed(
        const std::vector<Pickup>& candidates,
        size_t selectionCount)
    {
        std::vector<size_t> selectedIndices;

        if (selectionCount == 0 || candidates.empty())
            return selectedIndices;

        selectionCount =
            std::min(selectionCount, candidates.size());

        selectedIndices.reserve(selectionCount);

        // Pick the first candidate pseudo-randomly.
        const size_t firstIndex =
            static_cast<size_t>(random(candidates.size()));

        selectedIndices.push_back(firstIndex);

        while (selectedIndices.size() < selectionCount)
        {
            double bestDistance = -1.0;
            size_t bestIndex = 0;

            for (size_t candidateIndex = 0;
                 candidateIndex < candidates.size();
                 ++candidateIndex)
            {
                bool alreadySelected = false;

                for (size_t selectedIndex : selectedIndices)
                {
                    if (selectedIndex == candidateIndex)
                    {
                        alreadySelected = true;
                        break;
                    }
                }

                if (alreadySelected)
                    continue;

                double nearestDistance =
                    std::numeric_limits<double>::max();

                for (size_t selectedIndex : selectedIndices)
                {
                    nearestDistance =
                        std::min(
                            nearestDistance,
                            DistanceSquared(
                                candidates[candidateIndex],
                                candidates[selectedIndex]));
                }

                if (nearestDistance > bestDistance)
                {
                    bestDistance = nearestDistance;
                    bestIndex = candidateIndex;
                }
            }

            selectedIndices.push_back(bestIndex);
        }

        return selectedIndices;
    }

    void SelectPickupGroup(
        Zone& zone,
        PickupTypes type)
    {
        std::vector<Pickup> candidates;
        candidates.reserve(zone.pickups.size());

        for (const auto& entry : zone.pickups)
        {
            const Pickup& pickup = entry.second;

            if (pickup.type == type)
                candidates.push_back(pickup);
        }

        const size_t selectionCount =
            CalculateSelectionCount(candidates.size());

        if (selectionCount == 0)
            return;

        if (selectionCount >= candidates.size())
            return;

        const std::vector<size_t> selectedIndices =
            SelectEvenlyDistributed(
                candidates,
                selectionCount);

        std::vector<uint32_t> selectedIds;
        selectedIds.reserve(selectedIndices.size());

        for (size_t index : selectedIndices)
            selectedIds.push_back(candidates[index].id);

        for (auto it = zone.pickups.begin();
             it != zone.pickups.end();)
        {
            if (it->second.type == type)
            {
                const bool keep =
                    std::find(
                        selectedIds.begin(),
                        selectedIds.end(),
                        it->second.id) != selectedIds.end();

                if (!keep)
                {
                    it = zone.pickups.erase(it);
                    continue;
                }
            }

            ++it;
        }
    }

    /*
     * Calculate exact Health/Ammo/Weapon counts using the largest
     * remainder method. This guarantees that the resulting counts add
     * up exactly to the number of selected Auto pickups.
     */
    void CalculateAutoTypeCounts(
        size_t total,
        const PickupDistribution& distribution,
        size_t& healthCount,
        size_t& ammoCount,
        size_t& weaponCount)
    {
        healthCount = 0;
        ammoCount = 0;
        weaponCount = 0;

        const unsigned totalPercentage =
            static_cast<unsigned>(distribution.health) +
            static_cast<unsigned>(distribution.ammo) +
            static_cast<unsigned>(distribution.weapon);

        if (total == 0 || totalPercentage == 0)
            return;

        const double healthExact =
            static_cast<double>(total) *
            distribution.health / totalPercentage;

        const double ammoExact =
            static_cast<double>(total) *
            distribution.ammo / totalPercentage;

        const double weaponExact =
            static_cast<double>(total) *
            distribution.weapon / totalPercentage;

        healthCount =
            static_cast<size_t>(std::floor(healthExact));

        ammoCount =
            static_cast<size_t>(std::floor(ammoExact));

        weaponCount =
            static_cast<size_t>(std::floor(weaponExact));

        struct Remainder
        {
            double value;
            int type;
        };

        Remainder remainders[] =
        {
            { healthExact - healthCount, 0 },
            { ammoExact - ammoCount, 1 },
            { weaponExact - weaponCount, 2 }
        };

        size_t assigned =
            healthCount + ammoCount + weaponCount;

        std::sort(
            std::begin(remainders),
            std::end(remainders),
            [](const Remainder& a, const Remainder& b)
            {
                return a.value > b.value;
            });

        size_t remainderIndex = 0;

        while (assigned < total)
        {
            switch (remainders[remainderIndex].type)
            {
                case 0:
                    ++healthCount;
                    break;

                case 1:
                    ++ammoCount;
                    break;

                case 2:
                    ++weaponCount;
                    break;
            }

            ++assigned;
            remainderIndex =
                (remainderIndex + 1) % 3;
        }
    }

    void AssignAutoPickupTypes(
        Zone& zone,
        const PickupDistribution& distribution)
    {
        std::vector<int> autoIds;

        for (const auto& entry : zone.pickups)
        {
            if (entry.second.type == Auto)
                autoIds.push_back(entry.first);
        }

        if (autoIds.empty())
            return;

        size_t healthCount = 0;
        size_t ammoCount = 0;
        size_t weaponCount = 0;

        CalculateAutoTypeCounts(
            autoIds.size(),
            distribution,
            healthCount,
            ammoCount,
            weaponCount);

        // Shuffle the selected Auto pickups before assigning their
        // exact type counts, so the resulting types are pseudo-randomly
        // distributed amongst the selected locations.
        for (size_t i = autoIds.size(); i > 1; --i)
        {
            const size_t j =
                static_cast<size_t>(random(i));

            std::swap(autoIds[i - 1], autoIds[j]);
        }

        size_t index = 0;

        for (size_t i = 0; i < healthCount; ++i)
            zone.pickups[autoIds[index++]].type = Health;

        for (size_t i = 0; i < ammoCount; ++i)
            zone.pickups[autoIds[index++]].type = Ammo;

        for (size_t i = 0; i < weaponCount; ++i)
            zone.pickups[autoIds[index++]].type = WeaponBonus;
    }

    void AllocateZonePickups(
        Zone& zone,
        size_t zoneIndex)
    {
        const PickupDistribution& distribution =
            (zoneIndex < PickupDistributionCount)
                ? PickupDistributions[zoneIndex]
                : DefaultPickupDistribution;

        /*
         * Fixed pickup groups are allocated independently.
         */
        SelectPickupGroup(zone, Health);
        SelectPickupGroup(zone, Ammo);
        SelectPickupGroup(zone, WeaponBonus);

        /*
         * Auto is allocated independently from the fixed groups.
         */
        SelectPickupGroup(zone, Auto);

        /*
         * Auto pickups have no delayed spawn. They are present from
         * the moment their zone becomes active.
         */
        for (auto& entry : zone.pickups)
        {
            if (entry.second.type == Auto)
                entry.second.spawnTimeSeconds = 0;
        }

        /*
         * Convert the selected Auto pickups to their actual types.
         * Their spawn time remains zero.
         */
        AssignAutoPickupTypes(zone, distribution);
    }
}


bool GamePlan::Load(const char* filename)
{
    _version = 0;
    _homeLatitude = 0.0;
    _homeLongitude = 0.0;
    _valid = false;
    _zones.clear();

    if (filename == nullptr)
        return false;

    /*
     * Use the application's filesystem. The gameplan is expected to
     * have been mounted before Load() is called.
     *
     * This initial implementation uses LittleFS. If CombatLR is using
     * another filesystem for its map assets, this can be switched
     * without changing the GamePlan interface.
     */
    File file = LittleFS.open(filename, FILE_READ);

    if (!file)
    {
        Serial.printf(
            "GamePlan: failed to open %s\n",
            filename);

        return false;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(document, file);

    file.close();

    if (error)
    {
        Serial.printf(
            "GamePlan: JSON parse failed: %s\n",
            error.c_str());

        return false;
    }

    /*
     * version
     */
    if (!document["version"].is<int>())
    {
        Serial.println(
            "GamePlan: missing or invalid version");

        return false;
    }

    /*
     * home
     */
    JsonObject home =
        document["home"].as<JsonObject>();

    if (home.isNull())
    {
        Serial.println(
            "GamePlan: missing home");

        return false;
    }

    if (!home["latitude"].is<double>() ||
        !home["longitude"].is<double>())
    {
        Serial.println(
            "GamePlan: invalid home coordinates");

        return false;
    }

    _version =
        document["version"].as<int>();

    _homeLatitude =
        home["latitude"].as<double>();

    _homeLongitude =
        home["longitude"].as<double>();

    _valid = true;

    JsonArray zones =
        document["zones"].as<JsonArray>();

    if (zones.isNull())
    {
        Serial.println(
            "GamePlan: missing zones");

        return false;
    }

    for (JsonObject zoneJson : zones)
    {
        Zone zone;

        zone.eastMetres =
            zoneJson["eastMetres"].as<double>();

        zone.northMetres =
            zoneJson["northMetres"].as<double>();

        zone.radiusMetres =
            zoneJson["radiusMetres"].as<double>();

        zone.startTime =
            zoneJson["startTime"].as<uint32_t>();

        zone.lifespanSeconds =
            zoneJson["lifespanSeconds"].as<uint32_t>();

        zone.id = ZoneWidgetIdBase + _zones.size();

        JsonArray pickups =
            zoneJson["pickups"].as<JsonArray>();

        if (!pickups.isNull())
        {
            for (JsonObject pickupJson : pickups)
            {
                Pickup pickup;

                pickup.eastMetres =
                    pickupJson["eastMetres"].as<double>();

                pickup.northMetres =
                    pickupJson["northMetres"].as<double>();

                int intType =
                    pickupJson["type"].as<int>();

                pickup.type =
                    static_cast<PickupTypes>(intType);

                pickup.spawnTimeSeconds =
                    pickupJson["spawnTimeSeconds"].as<uint32_t>();

                /*
                 * Auto pickups are always present from the start of
                 * their zone. Ignore any non-zero value in the JSON.
                 */
                if (pickup.type == Auto)
                    pickup.spawnTimeSeconds = 0;

                pickup.lifespanSeconds =
                    pickupJson["lifespanSeconds"].as<uint32_t>();

                pickup.id =
                    pickupJson["id"].as<int>();

                zone.pickups.emplace(
                    pickup.id,
                    pickup);
            }
        }

        AllocateZonePickups(
            zone,
            _zones.size());

        _zones.push_back(
            std::move(zone));
    }

    Serial.printf(
        "GamePlan: loaded version %d\n",
        _version);

    Serial.printf(
        "GamePlan: home %.8f, %.8f\n",
        _homeLatitude,
        _homeLongitude);

    return true;
}

bool GamePlan::IsValid() const
{
    return _valid;
}

int GamePlan::GetVersion() const
{
    return _version;
}

double GamePlan::GetHomeLatitude() const
{
    return _homeLatitude;
}

double GamePlan::GetHomeLongitude() const
{
    return _homeLongitude;
}

const std::vector<Zone>& GamePlan::GetZones() const
{
    return _zones;
}
