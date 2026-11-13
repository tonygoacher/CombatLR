#pragma once

#include <Arduino.h>
#include "TeamIDs.h"

class SystemConfig
{
public:
    bool Begin();

    uint8_t GetTeamId() const;
    bool SetTeamId(uint8_t teamId);

    uint8_t GetPlayerId() const;
    bool SetPlayerId(uint8_t playerId);
    
    bool  GetFriendlyFire() const;
    bool  SetFriendlyFire(bool enabled);

    uint32_t GetGpsBaudRate() const { return _gpsBaudRate; }
    int GetGpsType() const { return _gpsType; }

    uint8_t GetWifiCount() const { return _wifiCount; }
    const String& GetWifiSSID(uint8_t index = 0) const { return _wifiNetworks[index].ssid; }
    const String& GetWifiPass(uint8_t index = 0) const { return _wifiNetworks[index].password; }
    uint8_t GetWifiMostRecent() const { return _wifiMostRecent; }
    bool SetWifiMostRecent(uint8_t index);
    bool SetProvisioningConfig(const String* ssids, const String* passwords, uint8_t count,
                               const String& ftpUser, const String& ftpPass);
    const String& GetFtpUser() { return _ftpUser;}
    const String& GetFtpPass() { return _ftpPass;}
    const char * const GetTeamFile(TeamIds teamId);

private:
    static constexpr const char* ConfigFile = "/system.json";

    uint8_t _teamId = 0;
    uint8_t _playerId = 0;
    bool    _friendlyFire;
    struct WifiNetwork
    {
        String ssid;
        String password;
    };

    static constexpr uint8_t MaxWifiNetworks = 8;
    WifiNetwork _wifiNetworks[MaxWifiNetworks];
    uint8_t _wifiCount = 0;
    uint8_t _wifiMostRecent = 0;
    String _ftpUser;
    String _ftpPass;
    uint32_t _gpsBaudRate = 9600;
    int _gpsType = 0;

    bool Load();
    bool Save() const;
};
