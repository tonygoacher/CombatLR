#pragma once

#include <Arduino.h>

class SystemConfig
{
public:
    bool Begin();

    uint8_t GetTeamId() const;
    bool SetTeamId(uint8_t teamId);

    uint8_t GetPlayerId() const;
    bool SetPlayerId(uint8_t playerId);

private:
    static constexpr const char* ConfigFile = "/system.json";

    uint8_t _teamId = 0;
    uint8_t _playerId = 0;

    bool Load();
    bool Save() const;
};
