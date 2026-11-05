#pragma once

#include <Arduino.h>
#include <map>
#include <vector>

#include "Zone.h"
#include "Pickup.h"

class IGameTime;





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
    const String& GetError() const;

    int GetVersion() const;
    const String& GetProjectName() const { return _projectName;}

    double GetHomeLatitude() const;
    double GetHomeLongitude() const;
    const std::vector<Zone>& GetZones() const;
    const int GetGameDelayTime() const;

    static constexpr int ZoneWidgetIdBase = 0xE000; // This should be below the GUI widget IDs so they are always on top
private:

    String _projectName;
    int _version = 0;
    int _gameDelayTime = 600;   // Default of 5 mins

    double _homeLatitude = 0.0;
    double _homeLongitude = 0.0;

    bool _valid = false;
    String _error;

    const GamePlan* _gamePlan;
    IGameTime* _gameTime;

    std::vector<Zone> _zones;
};
