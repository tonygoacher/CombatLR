#pragma once

#include <Arduino.h>
#include <map>
#include <vector>

#include "Zone.h"
#include "Pickup.h"




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
    float GetPickupRadius() const;

    static constexpr int ZoneWidgetIdBase = 0xE000; // This should be below the GUI widget IDs so they are always on top
private:

    String _projectName;
    int _version = 0;
    int _gameDelayTime = 600;   // Default of 5 mins
    float _pickupRadius = 2.0f;

    double _homeLatitude = 0.0;
    double _homeLongitude = 0.0;

    bool _valid = false;
    String _error;

    const GamePlan* _gamePlan;

    std::vector<Zone> _zones;
};
