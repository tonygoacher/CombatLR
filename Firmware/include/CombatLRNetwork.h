#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

#include <memory>

#include "SystemConfig.h"

enum class CombatLRNetworkEventType
{
    TeamChanged,
    PlayerChanged,
    StartGame
};

class CombatLRNetworkEvent
{
public:
    explicit CombatLRNetworkEvent(
        CombatLRNetworkEventType type)
        : _type(type)
    {
    }

    virtual ~CombatLRNetworkEvent() = default;

    CombatLRNetworkEventType GetType() const
    {
        return _type;
    }

private:
    CombatLRNetworkEventType _type;
};

class TeamChangedEvent : public CombatLRNetworkEvent
{
public:
    explicit TeamChangedEvent(
        uint8_t teamId)
        : CombatLRNetworkEvent(
              CombatLRNetworkEventType::TeamChanged),
          _teamId(teamId)
    {
    }

    uint8_t GetTeamId() const
    {
        return _teamId;
    }

private:
    uint8_t _teamId;
};

class PlayerChangedEvent : public CombatLRNetworkEvent
{
public:
    explicit PlayerChangedEvent(
        uint8_t playerId)
        : CombatLRNetworkEvent(
              CombatLRNetworkEventType::PlayerChanged),
          _playerId(playerId)
    {
    }

    uint8_t GetPlayerId() const
    {
        return _playerId;
    }

private:
    uint8_t _playerId;
};

class StartGameCommand : public CombatLRNetworkEvent
{
public:
    StartGameCommand(
        int sequence,
        const char* gameStart)
        : CombatLRNetworkEvent(
              CombatLRNetworkEventType::StartGame),
          _sequence(sequence),
          _gameStart(gameStart)
    {
    }

    int GetSequence() const
    {
        return _sequence;
    }

    const String& GetGameStart() const
    {
        return _gameStart;
    }

private:
    int _sequence;
    String _gameStart;
};

class CombatLRNetwork
{
public:
    explicit CombatLRNetwork(
        SystemConfig& config);

    bool Begin(
        uint16_t port = 4210);

    void Stop();

    std::unique_ptr<CombatLRNetworkEvent> Update();

private:
    static constexpr uint8_t ProtocolVersion = 1;
    static constexpr uint8_t MaxTeamId = 3;

    void HandlePacket();

    std::unique_ptr<CombatLRNetworkEvent>
    HandleJson(
        JsonDocument& request);

    std::unique_ptr<CombatLRNetworkEvent>
    HandleSetTeam(
        JsonDocument& request);

    std::unique_ptr<CombatLRNetworkEvent>
    HandleSetPlayer(
        JsonDocument& request);

    std::unique_ptr<CombatLRNetworkEvent>
    HandleStartGame(
        JsonDocument& request);

    void HandleDiscovery(
        JsonDocument& request);

    void SendDeviceResponse();

    void SendAck(
        int sequence,
        bool success,
        const char* message);

    void SendJson(
        JsonDocument& response);

    WiFiUDP _udp;
    SystemConfig& _config;

    uint16_t _port = 4210;
    bool _started = false;

    std::unique_ptr<CombatLRNetworkEvent> _pendingEvent;
};
