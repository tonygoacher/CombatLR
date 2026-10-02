#include "SystemConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace
{
    constexpr uint8_t MaxTeamId = 3;
}

bool SystemConfig::Begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("SystemConfig: LittleFS mount failed");
        return false;
    }

    if (Load())
        return true;

    _teamId = 0;
    _playerId = 0;

    if (!Save())
    {
        Serial.println("SystemConfig: failed to create default configuration");
        return false;
    }

    return true;
}

bool SystemConfig::Load()
{
    if (!LittleFS.exists(ConfigFile))
        return false;

    File file = LittleFS.open(ConfigFile, "r");

    if (!file)
        return false;

    JsonDocument document;

    DeserializationError error =
        deserializeJson(document, file);

    file.close();

    if (error)
    {
        Serial.print("SystemConfig: JSON error: ");
        Serial.println(error.c_str());
        return false;
    }

    int teamId = document["teamId"] | 0;
    int playerId = document["playerId"] | 0;

    if (teamId < 0 || teamId > MaxTeamId)
        return false;

    if (playerId < 0 || playerId > 255)
        return false;

    _teamId = static_cast<uint8_t>(teamId);
    _playerId = static_cast<uint8_t>(playerId);
    _friendlyFire = document["friendlyFire"] | false;

    Serial.print("SystemConfig: Team ID = ");
    Serial.println(_teamId);

    Serial.print("SystemConfig: Player ID = ");
    Serial.println(_playerId);

    Serial.print("SystemConfig: Friendly Fire = ");
    Serial.println(_friendlyFire);

    return true;
}

bool SystemConfig::Save() const
{
    File file = LittleFS.open(ConfigFile, "w");

    if (!file)
        return false;

    JsonDocument document;

    document["teamId"] = _teamId;
    document["playerId"] = _playerId;
    document["friendlyFire"] = _friendlyFire;

    if (serializeJson(document, file) == 0)
    {
        file.close();
        return false;
    }

    file.close();

    Serial.print("SystemConfig: saved Team ID = ");
    Serial.print(_teamId);
    Serial.print(", Player ID = ");
    Serial.print(_playerId);
    Serial.print(", Friendly fire = ");
    Serial.println(_friendlyFire);    

    return true;
}

uint8_t SystemConfig::GetTeamId() const
{
    return _teamId;
}

bool SystemConfig::SetTeamId(uint8_t teamId)
{
    if (teamId > MaxTeamId)
        return false;

    if (teamId == _teamId)
        return true;

    uint8_t oldTeamId = _teamId;
    _teamId = teamId;

    if (Save())
        return true;

    _teamId = oldTeamId;
    return false;
}

uint8_t SystemConfig::GetPlayerId() const
{
    return _playerId;
}

bool SystemConfig::SetPlayerId(uint8_t playerId)
{
    if (playerId == _playerId)
        return true;

    uint8_t oldPlayerId = _playerId;
    _playerId = playerId;

    if (Save())
        return true;

    _playerId = oldPlayerId;
    return false;
}



bool SystemConfig::GetFriendlyFire() const
{
    return _friendlyFire;
}



bool SystemConfig::SetFriendlyFire(bool enabled)
{
    bool oldFF = _friendlyFire;
    _friendlyFire = enabled;
    Serial.printf("Config setting FF to %d\n", enabled);
    if(Save())
    {
        return true;
    }
    _friendlyFire = oldFF;
    return false;
}
