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
    // Legacy game plans default to selecting 40% of candidate pickups.
    constexpr uint8_t DefaultPickupAllocationPercentage = 40;

    struct PickupDistribution
    {
        uint8_t health;
        uint8_t ammo;
        uint8_t weapon;
    };

    struct ZonePickupDistribution
    {
        size_t zone;
        size_t sourceIndex;
        uint8_t allocationPercentage;
        PickupDistribution distribution;
    };

    constexpr PickupDistribution DefaultPickupDistribution = { 40, 40, 20 };

    bool IsFiniteNumber(JsonVariantConst value)
    {
        return value.is<double>() && std::isfinite(value.as<double>());
    }

    bool IsUnsignedInteger(JsonVariantConst value)
    {
        if (value.is<unsigned int>())
            return true;

        return value.is<int>() && value.as<int>() >= 0;
    }

    bool IsPercentage(JsonVariantConst value)
    {
        return value.is<int>() && value.as<int>() >= 0 && value.as<int>() <= 100;
    }

    String ArrayItemError(const char* arrayName, size_t index, const char* reason)
    {
        return String(arrayName) + "[" + String(static_cast<unsigned int>(index)) + "]: " + reason;
    }

    double DistanceSquared(const Pickup& a, const Pickup& b)
    {
        const double dx = a.eastMetres - b.eastMetres;
        const double dy = a.northMetres - b.northMetres;

        return (dx * dx) + (dy * dy);
    }

    size_t CalculateSelectionCount(size_t candidateCount, uint8_t allocationPercentage)
    {
        if (candidateCount == 0 || allocationPercentage == 0)
            return 0;

        const double selected =
            std::round(
                static_cast<double>(candidateCount) *
                static_cast<double>(allocationPercentage) /
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
        PickupTypes type,
        uint8_t allocationPercentage)
    {
        std::vector<Pickup> candidates;
        candidates.reserve(zone.pickups.size());

        for (const auto& entry : zone.pickups)
        {
            const Pickup& pickup = entry.second;

            if (pickup.type == type)
                candidates.push_back(pickup);
        }

        if (candidates.empty())
            return;

        const size_t selectionCount =
            CalculateSelectionCount(candidates.size(), allocationPercentage);

        if (selectionCount == 0)
        {
            // A configured 0% means no candidates of this type are retained.
            for (auto it = zone.pickups.begin(); it != zone.pickups.end();)
            {
                if (it->second.type == type)
                    it = zone.pickups.erase(it);
                else
                    ++it;
            }
            return;
        }

        if (selectionCount >= candidates.size())
            return;

        const std::vector<size_t> selectedIndices =
            SelectEvenlyDistributed(
                candidates,
                selectionCount);

        std::vector<uint32_t> selectedIds;
        Serial.printf("Selected indececs %d\n", selectedIndices.size());
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
        const PickupDistribution& distribution,
        uint8_t allocationPercentage)
    {

        /*
         * Fixed pickup groups are allocated independently.
         */
        SelectPickupGroup(zone, Health, allocationPercentage);
        SelectPickupGroup(zone, Ammo, allocationPercentage);
        SelectPickupGroup(zone, WeaponBonus, allocationPercentage);

        /*
         * Auto is allocated independently from the fixed groups.
         */
        SelectPickupGroup(zone, Auto, allocationPercentage);

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
    _error = "";
    _version = 0;
    _projectName = "Untitled";
    _gameDelayTime = 600;
    _homeLatitude = 0.0;
    _homeLongitude = 0.0;
    _valid = false;
    _zones.clear();

    if (filename == nullptr)
    {
        _error = "No game plan filename was provided";
        return false;
    }

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
        _error = String("Failed to open game plan file: ") + filename;
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
        _error = String("JSON parse failed: ") + error.c_str();
        Serial.printf(
            "GamePlan: JSON parse failed: %s\n",
            error.c_str());

        return false;
    }

    /*
     * version
     */
    if (!document["version"].is<int>() || document["version"].as<int>() < 0)
    {
        _error = "Missing or invalid version";
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
        _error = "Missing home object";
        Serial.println(
            "GamePlan: missing home");

        return false;
    }

    if (!IsFiniteNumber(home["latitude"]) ||
        !IsFiniteNumber(home["longitude"]) ||
        home["latitude"].as<double>() < -90.0 || home["latitude"].as<double>() > 90.0 ||
        home["longitude"].as<double>() < -180.0 || home["longitude"].as<double>() > 180.0)
    {
        _error = "Invalid home coordinates";
        Serial.println(
            "GamePlan: invalid home coordinates");

        return false;
    }

    _version =
        document["version"].as<int>();

    if (!document["projectName"].isNull() && !document["projectName"].is<const char*>())
    {
        _error = "Invalid projectName; expected a string";
        Serial.println("GamePlan: invalid projectName");
        return false;
    }
    _projectName = document["projectName"].as<String>();
    if (_projectName.length() == 0)
    {
          _projectName="Untitled";
    }


    _homeLatitude =
        home["latitude"].as<double>();

    _homeLongitude =
        home["longitude"].as<double>();

        
    if (!document["gameDelayTime"].isNull())
    {
        if (!document["gameDelayTime"].is<int>() || document["gameDelayTime"].as<int>() < 0)
        {
            _error = "Invalid gameDelayTime; expected a non-negative integer";
            Serial.println("GamePlan: invalid gameDelayTime");
            return false;
        }
        _gameDelayTime = document["gameDelayTime"].as<int>();
    }

    std::vector<ZonePickupDistribution> pickupDistributions;
    JsonArray distributionJson = document["zonePercentages"].as<JsonArray>();

    if (distributionJson.isNull())
    {
        _error = "Missing zonePercentages array";
        Serial.println("GamePlan: missing zonePercentages array");
        return false;
    }

    size_t distributionIndex = 0;
    for (JsonObject entry : distributionJson)
    {
        if (!entry["zone"].is<int>() ||
            (!entry["allocationPercentage"].isNull() && !entry["allocationPercentage"].is<int>()) ||
            !IsPercentage(entry["healthPercentage"]) ||
            !IsPercentage(entry["ammoPercentage"]) ||
            !IsPercentage(entry["weaponPercentage"]) ||
            (!entry["allocationPercentage"].isNull() && !IsPercentage(entry["allocationPercentage"])))
        {
            _error = ArrayItemError("zonePercentages", distributionIndex, "invalid entry");
            Serial.println("GamePlan: invalid zonePercentages entry");
            return false;
        }

        const int zoneIndex = entry["zone"].as<int>();
        if (zoneIndex < 0)
        {
            _error = ArrayItemError("zonePercentages", distributionIndex, "zone cannot be negative");
            Serial.println("GamePlan: zonePercentages zone cannot be negative");
            return false;
        }

        ZonePickupDistribution configured;
        configured.zone = static_cast<size_t>(zoneIndex);
        configured.sourceIndex = distributionIndex;
        configured.allocationPercentage = entry["allocationPercentage"].is<int>()
            ? static_cast<uint8_t>(entry["allocationPercentage"].as<int>())
            : DefaultPickupAllocationPercentage;
        configured.distribution.health = static_cast<uint8_t>(entry["healthPercentage"].as<int>());
        configured.distribution.ammo = static_cast<uint8_t>(entry["ammoPercentage"].as<int>());
        configured.distribution.weapon = static_cast<uint8_t>(entry["weaponPercentage"].as<int>());
        pickupDistributions.push_back(configured);
        ++distributionIndex;
    }

    if (pickupDistributions.empty() ||
        std::none_of(pickupDistributions.begin(), pickupDistributions.end(),
            [](const ZonePickupDistribution& entry) { return entry.zone == 0; }))
    {
        _error = "zonePercentages must contain zone 0";
        Serial.println("GamePlan: zonePercentages must contain zone 0");
        return false;
    }

    std::sort(pickupDistributions.begin(), pickupDistributions.end(),
        [](const ZonePickupDistribution& a, const ZonePickupDistribution& b)
        {
            return a.zone < b.zone;
        });

    // A threshold may appear only once; duplicate zone IDs are ambiguous.
    for (size_t i = 1; i < pickupDistributions.size(); ++i)
    {
        if (pickupDistributions[i - 1].zone == pickupDistributions[i].zone)
        {
            _error = ArrayItemError(
                "zonePercentages",
                pickupDistributions[i].sourceIndex,
                "duplicate zone threshold");
            Serial.println("GamePlan: duplicate zone in zonePercentages");
            return false;
        }
    }
  

    JsonArray zones =
        document["zones"].as<JsonArray>();

    if (zones.isNull())
    {
        _error = "Missing zones array";
        Serial.println(
            "GamePlan: missing zones");

        return false;
    }

    size_t zoneArrayIndex = 0;
    for (JsonVariantConst zoneValue : zones)
    {
        if (!zoneValue.is<JsonObjectConst>())
        {
            _error = ArrayItemError("zones", zoneArrayIndex, "expected an object");
            Serial.println("GamePlan: invalid zone entry");
            return false;
        }
        JsonObjectConst zoneJson = zoneValue.as<JsonObjectConst>();
        if (!IsFiniteNumber(zoneJson["eastMetres"]) ||
            !IsFiniteNumber(zoneJson["northMetres"]) ||
            !IsFiniteNumber(zoneJson["radiusMetres"]) ||
            zoneJson["radiusMetres"].as<double>() <= 0.0 ||
            !IsUnsignedInteger(zoneJson["startTime"]) ||
            !IsUnsignedInteger(zoneJson["lifespanSeconds"]))
        {
            _error = ArrayItemError("zones", zoneArrayIndex, "invalid fields or values");
            Serial.println("GamePlan: invalid zone fields");
            return false;
        }

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

        JsonVariantConst pickupsValue = zoneJson["pickups"];

        if (!pickupsValue.isNull() && !pickupsValue.is<JsonArrayConst>())
        {
            _error = ArrayItemError("zones", zoneArrayIndex, "pickups must be an array");
            Serial.println("GamePlan: invalid pickups array");
            return false;
        }
        if (!pickupsValue.isNull())
        {
            JsonArrayConst pickups = pickupsValue.as<JsonArrayConst>();
            size_t pickupArrayIndex = 0;
            for (JsonVariantConst pickupValue : pickups)
            {
                if (!pickupValue.is<JsonObjectConst>())
                {
                    _error = ArrayItemError("pickups", pickupArrayIndex, "expected an object");
                    Serial.println("GamePlan: invalid pickup entry");
                    return false;
                }
                JsonObjectConst pickupJson = pickupValue.as<JsonObjectConst>();
                if (!IsFiniteNumber(pickupJson["eastMetres"]) ||
                    !IsFiniteNumber(pickupJson["northMetres"]) ||
                    !pickupJson["type"].is<int>() ||
                    pickupJson["type"].as<int>() < Health ||
                    pickupJson["type"].as<int>() > Auto ||
                    !IsUnsignedInteger(pickupJson["spawnTimeSeconds"]) ||
                    !IsUnsignedInteger(pickupJson["lifespanSeconds"]) ||
                    !IsUnsignedInteger(pickupJson["id"]) ||
                    pickupJson["id"].as<unsigned int>() > static_cast<unsigned int>(std::numeric_limits<int>::max()))
                {
                    _error = ArrayItemError("pickups", pickupArrayIndex, "invalid fields or values");
                    Serial.println("GamePlan: invalid pickup fields");
                    return false;
                }

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

                if (!zone.pickups.emplace(
                    pickup.id,
                    pickup).second)
                {
                    _error = ArrayItemError("pickups", pickupArrayIndex, "duplicate pickup id in zone");
                    Serial.println("GamePlan: duplicate pickup id in zone");
                    return false;
                }
                ++pickupArrayIndex;
            }
        }

        // Apply the most recent configured distribution threshold. A
        // threshold remains active until a later configured zone is reached.
        const size_t zoneIndex = _zones.size();
        const ZonePickupDistribution* activeDistribution = nullptr;
        for (const auto& configured : pickupDistributions)
        {
            if (configured.zone > zoneIndex)
                break;

            activeDistribution = &configured;
        }

        AllocateZonePickups(
            zone,
            activeDistribution != nullptr
                ? activeDistribution->distribution
                : DefaultPickupDistribution,
            activeDistribution != nullptr
                ? activeDistribution->allocationPercentage
                : DefaultPickupAllocationPercentage);

        _zones.push_back(
            std::move(zone));
        ++zoneArrayIndex;
    }

    _valid = true;

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

const String& GamePlan::GetError() const
{
    return _error;
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



const int GamePlan::GetGameDelayTime() const
{
    return 2;//_gameDelayTime;
}
