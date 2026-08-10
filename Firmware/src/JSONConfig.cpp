#include "JSONConfig.h"

#include <string.h>


JSONConfig::JSONConfig(
    fs::FS& filesystem,
    const char* filename)
    :
    _filesystem(filesystem),
    _filename(filename),
    _config{}
{
}


/*
 * =============================================================
 * Load configuration
 * =============================================================
 */

bool JSONConfig::Load()
{
    /*
     * Start with a clean configuration.
     */
    _config = Config{};


    /*
     * Open configuration file.
     */
    File file =
        _filesystem.open(
            _filename,
            FILE_READ);


    if (!file)
    {
        Serial.printf(
            "JSONConfig: unable to open %s\n",
            _filename);

        return false;
    }


    /*
     * Parse JSON directly from the file.
     */
    JsonDocument document;

    DeserializationError error =
        deserializeJson(
            document,
            file);


    file.close();


    if (error)
    {
        Serial.printf(
            "JSONConfig: JSON error: %s\n",
            error.c_str());

        return false;
    }


    /*
     * Populate the strongly typed configuration.
     */
    if (!ParseConfig(document))
    {
        Serial.println(
            "JSONConfig: invalid configuration");

        return false;
    }


    return true;
}


/*
 * =============================================================
 * GetConfig
 * =============================================================
 */

const Config* JSONConfig::GetConfig() const
{
    return &_config;
}


/*
 * =============================================================
 * Parse complete configuration
 * =============================================================
 */

bool JSONConfig::ParseConfig(
    JsonDocument& document)
{
    JsonObjectConst root =
        document.as<JsonObjectConst>();


    if (root.isNull())
    {
        return false;
    }


    /*
     * ---------------------------------------------------------
     * Player
     * ---------------------------------------------------------
     */

    JsonObjectConst player =
        root["player"].as<JsonObjectConst>();


    if (player.isNull())
    {
        return false;
    }


    if (!ParsePlayer(player))
    {
        return false;
    }


    /*
     * ---------------------------------------------------------
     * Maximum game time
     * ---------------------------------------------------------
     */

    if (!root["maxGameTime"].is<uint32_t>())
    {
        return false;
    }


    _config.maxGameTime =
        root["maxGameTime"].as<uint32_t>();


    /*
     * ---------------------------------------------------------
     * Home position
     * ---------------------------------------------------------
     */

    JsonObjectConst home =
        root["homePosition"].as<JsonObjectConst>();


    if (home.isNull())
    {
        return false;
    }


    if (!ParseGPSPosition(
            home,
            _config.homePosition))
    {
        return false;
    }


    /*
     * ---------------------------------------------------------
     * Zones
     * ---------------------------------------------------------
     */

    JsonArrayConst zones =
        root["zones"].as<JsonArrayConst>();


    if (!zones.isNull())
    {
        if (!ParseZones(zones))
        {
            return false;
        }
    }


    /*
     * ---------------------------------------------------------
     * Pickups
     * ---------------------------------------------------------
     */

    JsonArrayConst pickups =
        root["pickups"].as<JsonArrayConst>();


    if (!pickups.isNull())
    {
        if (!ParsePickups(pickups))
        {
            return false;
        }
    }


    return true;
}


/*
 * =============================================================
 * Parse player configuration
 * =============================================================
 */

bool JSONConfig::ParsePlayer(
    JsonObjectConst object)
{
    /*
     * Team ID.
     */
    if (!object["teamId"].is<uint8_t>())
    {
        return false;
    }

    _config.player.teamId =
        object["teamId"].as<uint8_t>();


    /*
     * Gun ID.
     */
    if (!object["gunId"].is<uint16_t>())
    {
        return false;
    }

    _config.player.gunId =
        object["gunId"].as<uint16_t>();


    /*
     * Friendly fire.
     */
    if (!object["friendlyFire"].is<bool>())
    {
        return false;
    }

    _config.player.friendlyFire =
        object["friendlyFire"].as<bool>();


    /*
     * Starting weapon.
     */
    const char* weaponName =
        object["startWeapon"];


    if (weaponName == nullptr)
    {
        return false;
    }


    if (!ParseWeaponType(
            weaponName,
            _config.player.startWeapon))
    {
        return false;
    }


    /*
     * Starting health.
     */
    if (!object["startHealth"].is<uint16_t>())
    {
        return false;
    }

    _config.player.startHealth =
        object["startHealth"].as<uint16_t>();


    return true;
}


/*
 * =============================================================
 * Parse GPS position
 * =============================================================
 */

bool JSONConfig::ParseGPSPosition(
    JsonObjectConst object,
    GPSPosition& position)
{
    if (!object["latitude"].is<double>())
    {
        return false;
    }


    if (!object["longitude"].is<double>())
    {
        return false;
    }


    position.latitude =
        object["latitude"].as<double>();


    position.longitude =
        object["longitude"].as<double>();


    return true;
}


/*
 * =============================================================
 * Parse weapon type
 * =============================================================
 *
 * JSON uses names rather than enum numbers.
 */

bool JSONConfig::ParseWeaponType(
    const char* name,
    WeaponType& type)
{
    if (strcmp(name, "pistol") == 0)
    {
        type = WeaponType::Pistol;
        return true;
    }


    if (strcmp(name, "smg") == 0)
    {
        type = WeaponType::SMG;
        return true;
    }


    if (strcmp(name, "assault_rifle") == 0)
    {
        type = WeaponType::AssaultRifle;
        return true;
    }


    if (strcmp(name, "shotgun") == 0)
    {
        type = WeaponType::Shotgun;
        return true;
    }


    if (strcmp(name, "sniper") == 0)
    {
        type = WeaponType::Sniper;
        return true;
    }


    return false;
}


/*
 * =============================================================
 * Parse pickup type
 * =============================================================
 *
 * There aren't any pickup types yet.
 *
 * This function deliberately rejects everything for now.
 * When a pickup is added, it gets one entry here and one entry
 * in the PickupType enum.
 */

bool JSONConfig::ParsePickupType(
    const char* name,
    PickupType& type)
{
    (void)name;
    (void)type;

    return false;
}


/*
 * =============================================================
 * Parse zones
 * =============================================================
 */

bool JSONConfig::ParseZones(
    JsonArrayConst array)
{
    if (array.size() > MAX_ZONES)
    {
        Serial.printf(
            "JSONConfig: too many zones (%u)\n",
            static_cast<unsigned>(array.size()));

        return false;
    }


    _config.zoneCount = 0;


    for (JsonObjectConst object : array)
    {
        Zone& zone =
            _config.zones[
                _config.zoneCount];


        /*
         * Centre.
         */
        JsonObjectConst centre =
            object["centre"].as<JsonObjectConst>();


        if (centre.isNull())
        {
            return false;
        }


        if (!ParseGPSPosition(
                centre,
                zone.centre))
        {
            return false;
        }


        /*
         * Radius.
         */
        if (!object["radius"].is<uint16_t>())
        {
            return false;
        }


        zone.radius =
            object["radius"].as<uint16_t>();


        /*
         * Activation time.
         */
        if (!object["activationTime"].is<uint32_t>())
        {
            return false;
        }


        zone.activationTime =
            object["activationTime"].as<uint32_t>();


        ++_config.zoneCount;
    }


    return true;
}


/*
 * =============================================================
 * Parse pickups
 * =============================================================
 */

bool JSONConfig::ParsePickups(
    JsonArrayConst array)
{
    if (array.size() > MAX_PICKUPS)
    {
        Serial.printf(
            "JSONConfig: too many pickups (%u)\n",
            static_cast<unsigned>(array.size()));

        return false;
    }


    _config.pickupCount = 0;


    for (JsonObjectConst object : array)
    {
        Pickup& pickup =
            _config.pickups[
                _config.pickupCount];


        /*
         * Pickup type.
         */
        const char* pickupName =
            object["type"];


        if (pickupName == nullptr)
        {
            return false;
        }


        if (!ParsePickupType(
                pickupName,
                pickup.type))
        {
            return false;
        }


        /*
         * Position.
         */
        JsonObjectConst position =
            object["position"].as<JsonObjectConst>();


        if (position.isNull())
        {
            return false;
        }


        if (!ParseGPSPosition(
                position,
                pickup.position))
        {
            return false;
        }


        /*
         * Activation time.
         */
        if (!object["activationTime"].is<uint32_t>())
        {
            return false;
        }


        pickup.activationTime =
            object["activationTime"].as<uint32_t>();


        ++_config.pickupCount;
    }


    return true;
}