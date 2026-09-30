#include "CombatLRNetwork.h"

#include <WiFi.h>

CombatLRNetwork::CombatLRNetwork(
    SystemConfig& config)
    : _config(config)
{
}

bool CombatLRNetwork::Begin(
    uint16_t port)
{
    if (_started)
        return true;

    _port = port;

    if (!_udp.begin(_port))
    {
        Serial.print(
            "CombatLRNetwork: failed to start UDP on port ");
        Serial.println(_port);

        return false;
    }

    _started = true;

    Serial.print(
        "CombatLRNetwork: UDP listening on port ");
    Serial.println(_port);

    return true;
}

void CombatLRNetwork::Stop()
{
    if (!_started)
        return;

    _udp.stop();
    _started = false;
    _pendingEvent.reset();

    Serial.println(
        "CombatLRNetwork: stopped");
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::Update()
{
    if (!_started)
        return nullptr;

    if (_pendingEvent)
    {
        auto event =
            std::move(_pendingEvent);

        _pendingEvent.reset();

        return event;
    }

    if (_udp.parsePacket() <= 0)
        return nullptr;

    HandlePacket();

    if (!_pendingEvent)
        return nullptr;

    auto event =
        std::move(_pendingEvent);

    _pendingEvent.reset();

    return event;
}

void CombatLRNetwork::HandlePacket()
{
    char packet[256];

    int length =
        _udp.read(
            packet,
            sizeof(packet) - 1);

    if (length <= 0)
        return;

    packet[length] = '\0';

    Serial.print(
        "UDP DATA: ");
    Serial.println(packet);

    JsonDocument request;

    DeserializationError error =
        deserializeJson(
            request,
            packet);

    if (error)
    {
        Serial.print(
            "CombatLRNetwork: JSON error: ");
        Serial.println(
            error.c_str());

        return;
    }

    _pendingEvent =
        HandleJson(request);
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::HandleJson(
    JsonDocument& request)
{
    uint8_t version =
        request["version"] | 0;

    if (version != ProtocolVersion)
        return nullptr;

    const char* type =
        request["type"] | "";

    if (strcmp(
            type,
            "discover") == 0)
    {
        HandleDiscovery(request);
        return nullptr;
    }

    if (strcmp(
            type,
            "setTeam") == 0)
    {
        return HandleSetTeam(request);
    }

    if (strcmp(
            type,
            "setPlayer") == 0)
    {
        return HandleSetPlayer(request);
    }

    if (strcmp(
            type,
            "setStartTime") == 0)
    {
        return HandleSetStartTime(request);
    }

    if (strcmp(
            type,
            "startGame") == 0)
    {
        return HandleStartGame(request);
    }

    return nullptr;
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::HandleSetTeam(
    JsonDocument& request)
{
    int sequence =
        request["sequence"] | 0;

    if (!request["teamId"].is<int>())
    {
        SendAck(
            sequence,
            false,
            "Invalid teamId");

        return nullptr;
    }

    int teamId =
        request["teamId"].as<int>();

    if (teamId < 0 ||
        teamId > MaxTeamId)
    {
        SendAck(
            sequence,
            false,
            "Team ID must be 0-3");

        return nullptr;
    }

    if (!_config.SetTeamId(
            static_cast<uint8_t>(teamId)))
    {
        SendAck(
            sequence,
            false,
            "Failed to save team ID");

        return nullptr;
    }

    SendAck(
        sequence,
        true,
        "Team ID set");

    return std::make_unique<TeamChangedEvent>(
        static_cast<uint8_t>(teamId));
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::HandleSetPlayer(
    JsonDocument& request)
{
    int sequence =
        request["sequence"] | 0;

    if (!request["playerId"].is<int>())
    {
        SendAck(
            sequence,
            false,
            "Invalid playerId");

        return nullptr;
    }

    int playerId =
        request["playerId"].as<int>();

    if (playerId < 0 ||
        playerId > 255)
    {
        SendAck(
            sequence,
            false,
            "Player ID must be 0-255");

        return nullptr;
    }

    if (!_config.SetPlayerId(
            static_cast<uint8_t>(playerId)))
    {
        SendAck(
            sequence,
            false,
            "Failed to save player ID");

        return nullptr;
    }

    SendAck(
        sequence,
        true,
        "Player ID set");

    return std::make_unique<PlayerChangedEvent>(
        static_cast<uint8_t>(playerId));
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::HandleSetStartTime(
    JsonDocument& request)
{
    int sequence =
        request["sequence"] | 0;

    if (!request["startTime"].is<uint32_t>())
    {
        SendAck(
            sequence,
            false,
            "Invalid startTime");

        return nullptr;
    }

    uint32_t startTime =
        request["startTime"].as<uint32_t>();

    SendAck(
        sequence,
        true,
        "Start time set");

    return std::make_unique<StartTimeChangedEvent>(
        startTime);
}

std::unique_ptr<CombatLRNetworkEvent>
CombatLRNetwork::HandleStartGame(
    JsonDocument& request)
{
    int sequence =
        request["sequence"] | 0;

    SendAck(
        sequence,
        true,
        "Game start received");

    return std::make_unique<StartGameEvent>();
}

void CombatLRNetwork::HandleDiscovery(
    JsonDocument& request)
{
    (void)request;

    SendDeviceResponse();
}

void CombatLRNetwork::SendDeviceResponse()
{
    JsonDocument response;

    response["type"] =
        "device";

    response["version"] =
        ProtocolVersion;

    response["deviceId"] =
        WiFi.macAddress();

    response["teamId"] =
        _config.GetTeamId();

    response["playerId"] =
        _config.GetPlayerId();

    SendJson(response);
}

void CombatLRNetwork::SendAck(
    int sequence,
    bool success,
    const char* message)
{
    JsonDocument response;

    response["type"] =
        "ack";

    response["version"] =
        ProtocolVersion;

    response["sequence"] =
        sequence;

    response["success"] =
        success;

    response["message"] =
        message;

    SendJson(response);
}

void CombatLRNetwork::SendJson(
    JsonDocument& response)
{
    if (!_started)
        return;

    String json;

    serializeJson(
        response,
        json);

    _udp.beginPacket(
        _udp.remoteIP(),
        _udp.remotePort());

    _udp.print(json);

    _udp.endPacket();

    Serial.print(
        "UDP RESPONSE: ");
    Serial.println(json);
}
