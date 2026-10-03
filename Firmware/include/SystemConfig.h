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
    
    bool  GetFriendlyFire() const;
    bool  SetFriendlyFire(bool enabled);

    const String& GetWifiSSID() { return _wifiSSID;}
    const String& GetWifiPass() { return _wifiPass;}
    const String& GetFtpUser() { return _ftpUser;}
    const String GetFtpPass() { return _ftpPass;}

private:
    static constexpr const char* ConfigFile = "/system.json";

    uint8_t _teamId = 0;
    uint8_t _playerId = 0;
    bool    _friendlyFire;
    String _wifiSSID;
    String _wifiPass;
    String _ftpUser;
    String _ftpPass;

    bool Load();
    bool Save() const;
};
