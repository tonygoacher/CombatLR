#pragma once

#include <stdint.h>


/*
 * =============================================================
 * Configuration limits
 * =============================================================
 */

static constexpr uint8_t MAX_ZONES   = 32;
static constexpr uint8_t MAX_PICKUPS = 32;


/*
 * Special team ID.
 *
 * 255 = no team. Accept hits from any team.
 */
static constexpr uint8_t NO_TEAM = 255;


/*
 * =============================================================
 * Weapon types
 * =============================================================
 *
 * These correspond to weapon definitions compiled into the
 * firmware.
 *
 * The numeric values are deliberately an implementation detail.
 * Configuration files use the names rather than these numbers.
 */
enum class WeaponType : uint8_t
{
    Pistol = 0,
    SMG,
    AssaultRifle,
    Shotgun,
    Sniper,

    Count
};


/*
 * =============================================================
 * Pickup types
 * =============================================================
 *
 * No pickup types are currently defined, but this provides the
 * framework for adding them later.
 */
enum class PickupType : uint8_t
{
    Count
};


/*
 * =============================================================
 * GPS position
 * =============================================================
 */

struct GPSPosition
{
    double latitude;
    double longitude;
};


/*
 * =============================================================
 * Zone
 * =============================================================
 *
 * activationTime is milliseconds from game start.
 */
struct Zone
{
    GPSPosition centre;

    uint16_t radius;

    uint32_t activationTime;
};


/*
 * =============================================================
 * Pickup
 * =============================================================
 *
 * activationTime is milliseconds from game start.
 */
struct Pickup
{
    PickupType type;

    GPSPosition position;

    uint32_t activationTime;
};


/*
 * =============================================================
 * Player configuration
 * =============================================================
 */

struct PlayerConfig
{
    uint8_t teamId;

    uint16_t gunId;

    bool friendlyFire;

    WeaponType startWeapon;

    uint16_t startHealth;
};


/*
 * =============================================================
 * Complete configuration
 * =============================================================
 */

struct Config
{
    /*
     * Player-specific configuration.
     */
    PlayerConfig player;


    /*
     * Maximum game duration in milliseconds.
     */
    uint32_t maxGameTime;


    /*
     * GPS coordinates of the site's home position.
     */
    GPSPosition homePosition;


    /*
     * Battlefield zones.
     */
    Zone zones[MAX_ZONES];

    uint8_t zoneCount;


    /*
     * Battlefield pickups.
     */
    Pickup pickups[MAX_PICKUPS];

    uint8_t pickupCount;
};


/*
 * =============================================================
 * Configuration interface
 * =============================================================
 */

class IConfig
{
public:

    virtual ~IConfig() = default;


    /*
     * Load the configuration.
     *
     * Returns true if successful.
     */
    virtual bool Load() = 0;


    /*
     * Return the loaded configuration.
     *
     * The caller receives a pointer to const data and therefore
     * cannot modify the configuration.
     */
    virtual const Config* GetConfig() const = 0;
};