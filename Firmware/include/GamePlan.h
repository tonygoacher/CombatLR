#pragma once

#include <Arduino.h>

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

private:
    int _version = 0;

    double _homeLatitude = 0.0;
    double _homeLongitude = 0.0;

    bool _valid = false;
};
