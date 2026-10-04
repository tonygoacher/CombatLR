#include "SystemConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace SystemConsts
{
    constexpr uint8_t MaxTeamId = 3;
    const char * const FriendlyFire = "friendlyFire";
    const char * const PlayerID =  "playerId";
    const char * const TeamID = "teamId";
    const char * const WifiSSID = "wifiSSID";
    const char * const WifiPass = "wifiPass";
    const char * const FTPUser = "ftpUser";
    const char * const FTPPass = "ftpPass";

    
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

    int teamId = document[SystemConsts::TeamID] | 0;
    int playerId = document[SystemConsts::PlayerID] | 0;

    if (teamId < 0 || teamId > SystemConsts::MaxTeamId)
        return false;

    if (playerId < 0 || playerId > 255)
        return false;

    _teamId = static_cast<uint8_t>(teamId);
    _playerId = static_cast<uint8_t>(playerId);
    _friendlyFire = document[SystemConsts::FriendlyFire] | false;

    _wifiSSID = static_cast<String>(document[SystemConsts::WifiSSID]);
    _wifiPass = static_cast<String>(document[SystemConsts::WifiPass]);    

    _ftpUser = static_cast<String>(document[SystemConsts::FTPUser]);
    _ftpPass = static_cast<String>(document[SystemConsts::FTPPass]);    


    Serial.print("SystemConfig: Team ID = ");
    Serial.println(_teamId);

    Serial.print("SystemConfig: Player ID = ");
    Serial.println(_playerId);

    Serial.print("SystemConfig: Friendly Fire = ");
    Serial.println(_friendlyFire);

    Serial.printf("WifiSSID %s\n", _wifiSSID);
    Serial.printf("WifiPass %s\n", _wifiPass);

    return true;
}

bool SystemConfig::Save() const
{
    File file = LittleFS.open(ConfigFile, "w");

    if (!file)
        return false;

    JsonDocument document;

    document[SystemConsts::TeamID] = _teamId;
    document[SystemConsts::PlayerID] = _playerId;
    document[SystemConsts::FriendlyFire] = _friendlyFire;
    document[SystemConsts::WifiSSID] = _wifiSSID;
    document[SystemConsts::WifiPass] = _wifiPass;
    document[SystemConsts::FTPUser] = _ftpUser;
    document[SystemConsts::FTPPass] = _ftpPass;


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
    Serial.printf("WifiSSID %s\n", _wifiSSID);
    Serial.printf("WifiPass %s\n", _wifiPass);

    return true;
}

uint8_t SystemConfig::GetTeamId() const
{
    return _teamId;
}

bool SystemConfig::SetTeamId(uint8_t teamId)
{
    if (teamId > SystemConsts::MaxTeamId)
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
