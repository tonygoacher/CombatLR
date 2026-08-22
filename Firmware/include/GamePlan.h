#pragma once

#include <Arduino.h>
#include <map>
#include <vector>

class IGameTime;

struct Pickup
{
    double eastMetres;
    double northMetres;

    int type;

    uint32_t spawnTimeSeconds;
    uint32_t lifespanSeconds;

    int id;
};

struct Zone
{
    double eastMetres;
    double northMetres;
    double radiusMetres;

    uint32_t startTime;
    uint32_t transitionTime;

    std::map<int, Pickup> pickups;
};

/*
 * Minimal GamePlan representation.
 *
 * The first implementation deliberately loads only the gameplan
 * information required to establish the fixed GPS reference:
 *
 *     home.latitude
 *     home.longitude
 *
 * Zones and pickups will be added when the game-plan/gameplay layer
 * is implemented.
 */
class GamePlan
{
public:

    bool Load(const char* filename);

    bool IsValid() const;

    int GetVersion() const;

    double GetHomeLatitude() const;
    double GetHomeLongitude() const;
    const std::vector<Zone>& GetZones() const;
private:

    int _version = 0;

    double _homeLatitude = 0.0;
    double _homeLongitude = 0.0;

    bool _valid = false;
    const GamePlan* _gamePlan;
    IGameTime* _gameTime;

    std::vector<Zone> _zones;
};
