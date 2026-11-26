#include "SystemConfig.h"

#include <LittleFS.h>

namespace SystemConsts
{
    constexpr uint8_t MaxTeamId = 3;
    const char * const FriendlyFire = "friendlyFire";
    const char * const PlayerID = "playerId";
    const char * const TeamID = "teamId";
    const char * const Wifi = "wifi";
    const char * const WifiSSID = "wifiSSID";       // legacy
    const char * const WifiPass = "wifiPass";       // legacy
    const char * const WifiMostRecent = "mostRecent";
    const char * const FTPUser = "ftpUser";
    const char * const FTPPass = "ftpPass";
    const char * const GPSBaudRate = "gpsBaudrate";
    const char * const GPSType = "gpsType";
}

namespace TeamIconFiles
{
    const char * const redTeam = "/teamred.bmp";
    const char * const blueTeam = "/teamblue.bmp";
    const char * const yellowTeam = "/teamyellow.bmp";
    const char * const greenTeam = "/teamgreen.bmp";
}

bool SystemConfig::Begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("SystemConfig: LittleFS mount failed");
        return false;
    }

    if (!_preferences.begin(CredentialsNamespace, false))
    {
        Serial.println("SystemConfig: NVS namespace open failed");
        return false;
    }
    _preferencesReady = true;

    if (!Load())
    {
        // Never replace an existing configuration when loading or credential
        // migration fails: it may still contain the only recoverable secrets.
        if (LittleFS.exists(ConfigFile))
            return false;

        _teamId = 0;
        _playerId = 0;
        _friendlyFire = false;
        _wifiCount = 0;
        _wifiMostRecent = 0;
        _gpsBaudRate = 9600;
        _gpsType = 0;

        // A device may have NVS credentials even if its filesystem config
        // file has been removed or freshly recreated.
        LoadCredentialsFromNvs();

        if (!Save())
        {
            Serial.println("SystemConfig: failed to create default configuration");
            return false;
        }
    }

    return true;
}

bool SystemConfig::LoadCredentialsFromNvs()
{
    if (!_preferencesReady || !_preferences.isKey(CredentialsKey))
        return false;

    String json = _preferences.getString(CredentialsKey, "");
    if (json.isEmpty())
        return false;

    JsonDocument document;
    if (deserializeJson(document, json))
    {
        Serial.println("SystemConfig: invalid credentials in NVS");
        return false;
    }

    _wifiCount = 0;
    if (document["wifi"].is<JsonArray>())
    {
        for (JsonObject network : document["wifi"].as<JsonArray>())
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

    _ftpUser = document["ftpUser"] | "";
    _ftpPass = document["ftpPass"] | "";
    return true;
}

bool SystemConfig::SaveCredentialsToNvs()
{
    if (!_preferencesReady)
        return false;

    JsonDocument document;
    JsonArray wifi = document["wifi"].to<JsonArray>();
    for (uint8_t i = 0; i < _wifiCount; ++i)
    {
        JsonObject network = wifi.add<JsonObject>();
        network["ssid"] = _wifiNetworks[i].ssid;
        network["password"] = _wifiNetworks[i].password;
    }
    document["ftpUser"] = _ftpUser;
    document["ftpPass"] = _ftpPass;

    String json;
    if (serializeJson(document, json) == 0)
        return false;

    return _preferences.putString(CredentialsKey, json) == json.length();
}

bool SystemConfig::MigrateCredentialsFromJson(JsonDocument& document)
{
    bool hasCredentials = document[SystemConsts::Wifi].is<JsonArray>() ||
                          document.containsKey(SystemConsts::WifiSSID) ||
                          document.containsKey(SystemConsts::FTPUser) ||
                          document.containsKey(SystemConsts::FTPPass);

    if (!hasCredentials)
        return true;

    _wifiCount = 0;
    if (document[SystemConsts::Wifi].is<JsonArray>())
    {
        for (JsonObject network : document[SystemConsts::Wifi].as<JsonArray>())
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

    _ftpUser = document[SystemConsts::FTPUser] | "";
    _ftpPass = document[SystemConsts::FTPPass] | "";

    // Never remove the legacy copy unless the NVS write has succeeded.
    return SaveCredentialsToNvs();
}

bool SystemConfig::Load()
{
    if (!LittleFS.exists(ConfigFile))
        return false;

    File file = LittleFS.open(ConfigFile, "r");
    if (!file)
        return false;

    JsonDocument document;
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error)
    {
        Serial.print("SystemConfig: JSON error: ");
        Serial.println(error.c_str());
        return false;
    }

    int teamId = document[SystemConsts::TeamID] | 0;
    int playerId = document[SystemConsts::PlayerID] | 0;
    if (teamId < 0 || teamId > SystemConsts::MaxTeamId || playerId < 0 || playerId > 255)
        return false;

    _teamId = static_cast<uint8_t>(teamId);
    _playerId = static_cast<uint8_t>(playerId);
    _friendlyFire = document[SystemConsts::FriendlyFire] | false;
    _wifiMostRecent = document[SystemConsts::WifiMostRecent] | 0;
    _gpsBaudRate = document[SystemConsts::GPSBaudRate] | 9600;
    _gpsType = document[SystemConsts::GPSType] | 0;

    bool credentialsLoaded = LoadCredentialsFromNvs();
    bool migrated = false;
    if (!credentialsLoaded)
    {
        if (!MigrateCredentialsFromJson(document))
        {
            Serial.println("SystemConfig: credential migration to NVS failed; retaining legacy JSON");
            return false;
        }
        migrated = true;
    }

    if (_wifiMostRecent >= _wifiCount)
        _wifiMostRecent = 0;

    // Rewrite legacy JSON without secrets only after NVS has a verified copy.
    if (migrated || document.containsKey(SystemConsts::Wifi) ||
        document.containsKey(SystemConsts::WifiSSID) ||
        document.containsKey(SystemConsts::FTPUser) ||
        document.containsKey(SystemConsts::FTPPass))
    {
        if (!Save())
            Serial.println("SystemConfig: could not sanitise system.json; credentials are safely stored in NVS and sanitisation will be retried next boot");
    }

    Serial.printf("SystemConfig: Team ID = %u\n", _teamId);
    Serial.printf("SystemConfig: Player ID = %u\n", _playerId);
    Serial.printf("SystemConfig: Friendly Fire = %u\n", _friendlyFire);
    Serial.printf("Wi-Fi networks configured = %u\n", _wifiCount);
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
    document[SystemConsts::GPSBaudRate] = _gpsBaudRate;
    document[SystemConsts::GPSType] = _gpsType;

    bool success = serializeJson(document, file) > 0;
    file.close();
    return success;
}

bool SystemConfig::SetWifiMostRecent(uint8_t index)
{
    if (_wifiCount == 0)
        return false;
    if (index >= _wifiCount)
        index = 0;
    if (index == _wifiMostRecent)
        return true;

    uint8_t old = _wifiMostRecent;
    _wifiMostRecent = index;
    if (Save())
        return true;
    _wifiMostRecent = old;
    return false;
}

bool SystemConfig::SetProvisioningConfig(
    const String* ssids, const String* passwords, uint8_t count,
    const String& ftpUser, const String& ftpPass)
{
    if (ssids == nullptr || passwords == nullptr || count == 0 || count > MaxWifiNetworks)
        return false;

    WifiNetwork oldNetworks[MaxWifiNetworks];
    const uint8_t oldCount = _wifiCount;
    const uint8_t oldMostRecent = _wifiMostRecent;
    const String oldFtpUser = _ftpUser;
    const String oldFtpPass = _ftpPass;
    for (uint8_t i = 0; i < MaxWifiNetworks; ++i)
        oldNetworks[i] = _wifiNetworks[i];

    _wifiCount = count;
    for (uint8_t i = 0; i < count; ++i)
    {
        _wifiNetworks[i].ssid = ssids[i];
        _wifiNetworks[i].password = passwords[i];
    }
    for (uint8_t i = count; i < MaxWifiNetworks; ++i)
    {
        _wifiNetworks[i].ssid = "";
        _wifiNetworks[i].password = "";
    }
    _wifiMostRecent = 0;
    _ftpUser = ftpUser;
    _ftpPass = ftpPass;

    if (SaveCredentialsToNvs())
    {
        if (Save())
            return true;
    }

    _wifiCount = oldCount;
    for (uint8_t i = 0; i < MaxWifiNetworks; ++i)
        _wifiNetworks[i] = oldNetworks[i];
    _wifiMostRecent = oldMostRecent;
    _ftpUser = oldFtpUser;
    _ftpPass = oldFtpPass;
    // Restore the previous NVS credentials if the filesystem save failed.
    SaveCredentialsToNvs();
    return false;
}

uint8_t SystemConfig::GetTeamId() const { return _teamId; }

bool SystemConfig::SetTeamId(uint8_t teamId)
{
    if (teamId > SystemConsts::MaxTeamId)
        return false;
    if (teamId == _teamId)
        return true;
    uint8_t old = _teamId;
    _teamId = teamId;
    if (Save())
        return true;
    _teamId = old;
    return false;
}

uint8_t SystemConfig::GetPlayerId() const { return _playerId; }

bool SystemConfig::SetPlayerId(uint8_t playerId)
{
    if (playerId == _playerId)
        return true;
    uint8_t old = _playerId;
    _playerId = playerId;
    if (Save())
        return true;
    _playerId = old;
    return false;
}

bool SystemConfig::GetFriendlyFire() const { return _friendlyFire; }

bool SystemConfig::SetFriendlyFire(bool enabled)
{
    bool old = _friendlyFire;
    _friendlyFire = enabled;
    if (Save())
        return true;
    _friendlyFire = old;
    return false;
}

const char * const SystemConfig::GetTeamFile(TeamIds teamId)
{
    switch (teamId)
    {
        case TeamIds::Red: return TeamIconFiles::redTeam;
        case TeamIds::Blue: return TeamIconFiles::blueTeam;
        case TeamIds::Yellow: return TeamIconFiles::yellowTeam;
        case TeamIds::Green: return TeamIconFiles::greenTeam;
        default:
            Serial.printf("SystemConfig::GetTeamFile Invalid team id %d\n", static_cast<int>(teamId));
            return nullptr;
    }
}
