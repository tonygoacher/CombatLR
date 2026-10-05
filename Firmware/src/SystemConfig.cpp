#include "SystemConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace SystemConsts
{
    constexpr uint8_t MaxTeamId = 3;
    const char * const FriendlyFire = "friendlyFire";
    const char * const PlayerID =  "playerId";
    const char * const TeamID = "teamId";
    const char * const Wifi = "wifi";
    const char * const WifiSSID = "wifiSSID";       // legacy
    const char * const WifiPass = "wifiPass";       // legacy
    const char * const WifiMostRecent = "mostRecent";
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

    _wifiCount = 0;
    _wifiMostRecent = document[SystemConsts::WifiMostRecent] | 0;

    // New format: an array of Wi-Fi networks.
    if (document[SystemConsts::Wifi].is<JsonArray>())
    {
        JsonArray wifi = document[SystemConsts::Wifi].as<JsonArray>();

        for (JsonObject network : wifi)
        {
            if (_wifiCount >= MaxWifiNetworks)
                break;

            const char* ssid = network["ssid"] | "";
            const char* password = network["password"] | "";

            if (ssid[0] == '\0')
                continue;

            _wifiNetworks[_wifiCount].ssid = ssid;
            _wifiNetworks[_wifiCount].password = password;
            ++_wifiCount;
        }
    }
    // Backwards compatibility with the original single-network format.
    else
    {
        const char* ssid = document[SystemConsts::WifiSSID] | "";
        const char* password = document[SystemConsts::WifiPass] | "";

        if (ssid[0] != '\0')
        {
            _wifiNetworks[0].ssid = ssid;
            _wifiNetworks[0].password = password;
            _wifiCount = 1;
        }
    }

    if (_wifiCount == 0)
    {
        Serial.println("SystemConfig: no Wi-Fi networks configured");
        return false;
    }

    if (_wifiMostRecent >= _wifiCount)
    {
        Serial.printf("SystemConfig: mostRecent %u is out of range, resetting to 0\n", _wifiMostRecent);
        _wifiMostRecent = 0;

        // Keep the JSON in sync with the corrected value.
        if (!Save())
            Serial.println("SystemConfig: failed to save corrected mostRecent");
    }

    _ftpUser = static_cast<String>(document[SystemConsts::FTPUser]);
    _ftpPass = static_cast<String>(document[SystemConsts::FTPPass]);    


    Serial.print("SystemConfig: Team ID = ");
    Serial.println(_teamId);

    Serial.print("SystemConfig: Player ID = ");
    Serial.println(_playerId);

    Serial.print("SystemConfig: Friendly Fire = ");
    Serial.println(_friendlyFire);

    Serial.printf("Wi-Fi networks = %u\n", _wifiCount);
    for (uint8_t i = 0; i < _wifiCount; ++i)
        Serial.printf("Wi-Fi %u: %s\n", i, _wifiNetworks[i].ssid.c_str());

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
    document[SystemConsts::WifiMostRecent] = _wifiMostRecent;
    JsonArray wifi = document[SystemConsts::Wifi].to<JsonArray>();
    for (uint8_t i = 0; i < _wifiCount; ++i)
    {
        JsonObject network = wifi.add<JsonObject>();
        network["ssid"] = _wifiNetworks[i].ssid;
        network["password"] = _wifiNetworks[i].password;
    }

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
    Serial.printf("Wi-Fi networks = %u\n", _wifiCount);
    for (uint8_t i = 0; i < _wifiCount; ++i)
        Serial.printf("Wi-Fi %u: %s\n", i, _wifiNetworks[i].ssid.c_str());

    return true;
}

bool SystemConfig::SetWifiMostRecent(uint8_t index)
{
    if (_wifiCount == 0)
        return false;

    if (index >= _wifiCount)
        index = 0;

    if (index == _wifiMostRecent)
        return true;

    const uint8_t oldMostRecent = _wifiMostRecent;
    _wifiMostRecent = index;

    if (Save())
        return true;

    _wifiMostRecent = oldMostRecent;
    return false;
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
