using System.Collections.Concurrent;
using System.ComponentModel;
using System.Globalization;
using System.Net;
using System.Net.Sockets;
using System.Net.NetworkInformation;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace CombatLR.mobile.Services;

public sealed class CombatLRNetworkService : IAsyncDisposable
{
    public const int DiscoveryPort = 4210;
    public const int ProtocolVersion = 1;
    public const int MaxTeamId = 3;
    public const int MaxPlayerId = 127;

    private const int DiscoveryIntervalMilliseconds = 2000;
    private const int CommandTimeoutMilliseconds = 2000;

    private readonly ConcurrentDictionary<string, CombatLRDevice> _devices = new();
    private readonly ConcurrentDictionary<int, TaskCompletionSource<CombatLRAck>> _pendingCommands = new();

    private UdpClient? _udp;
    private CancellationTokenSource? _cts;
    private Task? _discoveryTask;
    private int _nextSequence;

    public IReadOnlyCollection<CombatLRDevice> Devices =>
        _devices.Values.ToArray();

    public event EventHandler? DevicesChanged;

    public Task StartDiscoveryAsync()
    {
        if (_discoveryTask is not null)
            return Task.CompletedTask;

        _cts = new CancellationTokenSource();

        _udp = new UdpClient(0)
        {
            EnableBroadcast = true
        };

        _discoveryTask = DiscoveryLoopAsync(_cts.Token);

        return Task.CompletedTask;
    }

    public async Task StopDiscoveryAsync()
    {
        if (_cts is null)
            return;

        _cts.Cancel();

        _udp?.Dispose();
        _udp = null;

        foreach (var pending in _pendingCommands.Values)
            pending.TrySetCanceled();

        _pendingCommands.Clear();

        if (_discoveryTask is not null)
        {
            try
            {
                await _discoveryTask;
            }
            catch (OperationCanceledException)
            {
            }
        }

        _discoveryTask = null;

        _cts.Dispose();
        _cts = null;
    }

    public void ClearDevices()
    {
        _devices.Clear();
        DevicesChanged?.Invoke(this, EventArgs.Empty);
    }

    public async Task<bool> SetTeamIdAsync(
        CombatLRDevice device,
        int teamId)
    {
        if (_udp is null ||
            teamId < 0 ||
            teamId > MaxTeamId)
        {
            return false;
        }

        int sequence =
            Interlocked.Increment(ref _nextSequence);

        var command = new SetTeamCommand
        {
            Type = "setTeam",
            Version = ProtocolVersion,
            Sequence = sequence,
            TeamId = teamId
        };

        string json =
            JsonSerializer.Serialize(command);

        var completion =
            new TaskCompletionSource<CombatLRAck>(
                TaskCreationOptions.RunContinuationsAsynchronously);

        _pendingCommands[sequence] = completion;

        try
        {
            byte[] data =
                Encoding.UTF8.GetBytes(json);

            var endpoint = new IPEndPoint(
                IPAddress.Parse(device.IpAddress),
                DiscoveryPort);

            await _udp.SendAsync(
                data,
                data.Length,
                endpoint);

            using var timeout =
                new CancellationTokenSource(
                    CommandTimeoutMilliseconds);

            CombatLRAck ack;

            try
            {
                ack = await completion.Task.WaitAsync(
                    timeout.Token);
            }
            catch (OperationCanceledException)
            {
                return false;
            }

            if (!ack.Success)
                return false;

            device.TeamId = teamId;
            device.LastSeen = DateTimeOffset.UtcNow;

            return true;
        }
        finally
        {
            _pendingCommands.TryRemove(
                sequence,
                out _);
        }
    }

    public async Task<bool> SetPlayerIdAsync(
        CombatLRDevice device,
        int playerId)
    {
        if (_udp is null ||
            playerId < 0 ||
            playerId > MaxPlayerId)
        {
            return false;
        }

        int sequence =
            Interlocked.Increment(ref _nextSequence);

        var command = new SetPlayerCommand
        {
            Type = "setPlayer",
            Version = ProtocolVersion,
            Sequence = sequence,
            PlayerId = playerId
        };

        string json =
            JsonSerializer.Serialize(command);

        var completion =
            new TaskCompletionSource<CombatLRAck>(
                TaskCreationOptions.RunContinuationsAsynchronously);

        _pendingCommands[sequence] = completion;

        try
        {
            byte[] data =
                Encoding.UTF8.GetBytes(json);

            var endpoint = new IPEndPoint(
                IPAddress.Parse(device.IpAddress),
                DiscoveryPort);

            await _udp.SendAsync(
                data,
                data.Length,
                endpoint);

            using var timeout =
                new CancellationTokenSource(
                    CommandTimeoutMilliseconds);

            CombatLRAck ack;

            try
            {
                ack = await completion.Task.WaitAsync(
                    timeout.Token);
            }
            catch (OperationCanceledException)
            {
                return false;
            }

            if (!ack.Success)
                return false;

            device.PlayerId = playerId;
            device.LastSeen = DateTimeOffset.UtcNow;

            return true;
        }
        finally
        {
            _pendingCommands.TryRemove(
                sequence,
                out _);
        }
    }

    public async Task<StartGameResult> StartGameAsync()
    {
        if (_udp is null)
        {
            return new StartGameResult(
                "",
                Array.Empty<CombatLRDevice>(),
                _devices.Values.ToArray());
        }

        var devices = _devices.Values.ToArray();

        if (devices.Length == 0)
        {
            return new StartGameResult(
                "",
                Array.Empty<CombatLRDevice>(),
                Array.Empty<CombatLRDevice>());
        }

        // Capture the mobile device time once. Every ESP32 receives
        // exactly the same game start time.
        string gameStart =
            DateTime.UtcNow.ToString(
                "HH:mm:ss",
                CultureInfo.InvariantCulture);

        var results = await Task.WhenAll(
            devices.Select(async device =>
                new
                {
                    Device = device,
                    Success = await SendStartGameAsync(
                        device,
                        gameStart)
                }));

        var successfulDevices = results
            .Where(result => result.Success)
            .Select(result => result.Device)
            .ToArray();

        var failedDevices = results
            .Where(result => !result.Success)
            .Select(result => result.Device)
            .ToArray();

        return new StartGameResult(
            gameStart,
            successfulDevices,
            failedDevices);
    }

    private async Task<bool> SendStartGameAsync(
        CombatLRDevice device,
        string gameStart)
    {
        if (_udp is null)
            return false;

        int sequence =
            Interlocked.Increment(ref _nextSequence);

        var command = new StartGameCommand
        {
            Type = "startGame",
            Version = ProtocolVersion,
            Sequence = sequence,
            GameStart = gameStart
        };

        string json =
            JsonSerializer.Serialize(command);

        var completion =
            new TaskCompletionSource<CombatLRAck>(
                TaskCreationOptions.RunContinuationsAsynchronously);

        _pendingCommands[sequence] = completion;

        try
        {
            byte[] data =
                Encoding.UTF8.GetBytes(json);

            var endpoint = new IPEndPoint(
                IPAddress.Parse(device.IpAddress),
                DiscoveryPort);

            await _udp.SendAsync(
                data,
                data.Length,
                endpoint);

            using var timeout =
                new CancellationTokenSource(
                    CommandTimeoutMilliseconds);

            CombatLRAck ack;

            try
            {
                ack = await completion.Task.WaitAsync(
                    timeout.Token);
            }
            catch (OperationCanceledException)
            {
                return false;
            }

            if (!ack.Success)
                return false;

            device.LastSeen = DateTimeOffset.UtcNow;

            return true;
        }
        finally
        {
            _pendingCommands.TryRemove(
                sequence,
                out _);
        }
    }

    private async Task DiscoveryLoopAsync(
        CancellationToken cancellationToken)
    {
        var receiverTask =
            ReceiveLoopAsync(cancellationToken);

        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                await SendDiscoveryAsync(cancellationToken);

                await Task.Delay(
                    DiscoveryIntervalMilliseconds,
                    cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
        }

        try
        {
            await receiverTask;
        }
        catch (OperationCanceledException)
        {
        }
    }

    private async Task SendDiscoveryAsync(
        CancellationToken cancellationToken)
    {
        if (_udp is null)
            return;

        var request = new DiscoveryMessage
        {
            Type = "discover",
            Version = ProtocolVersion
        };

        string json =
            JsonSerializer.Serialize(request);

        byte[] data =
            Encoding.UTF8.GetBytes(json);

        var broadcastAddresses = GetBroadcastAddresses();

        foreach (var broadcastAddress in broadcastAddresses)
        {
            var endpoint = new IPEndPoint(
                broadcastAddress,
                DiscoveryPort);

            await _udp.SendAsync(
                data,
                data.Length,
                endpoint);
        }
    }

    private static IReadOnlyList<IPAddress> GetBroadcastAddresses()
    {
        var addresses = new List<IPAddress>();

        foreach (var networkInterface in NetworkInterface.GetAllNetworkInterfaces())
        {
            if (networkInterface.OperationalStatus != OperationalStatus.Up)
                continue;

            if (networkInterface.NetworkInterfaceType == NetworkInterfaceType.Loopback)
                continue;

            IPInterfaceProperties properties;

            try
            {
                properties = networkInterface.GetIPProperties();
            }
            catch
            {
                continue;
            }

            foreach (var unicast in properties.UnicastAddresses)
            {
                var address = unicast.Address;
                var mask = unicast.IPv4Mask;

                if (address.AddressFamily != AddressFamily.InterNetwork ||
                    mask is null)
                {
                    continue;
                }

                var addressBytes = address.GetAddressBytes();
                var maskBytes = mask.GetAddressBytes();
                var broadcastBytes = new byte[4];

                for (int i = 0; i < 4; i++)
                {
                    broadcastBytes[i] =
                        (byte)(addressBytes[i] | (byte)~maskBytes[i]);
                }

                var broadcast = new IPAddress(broadcastBytes);

                if (!addresses.Contains(broadcast))
                    addresses.Add(broadcast);
            }
        }

        if (addresses.Count == 0)
            addresses.Add(IPAddress.Broadcast);

        return addresses;
    }

    private async Task ReceiveLoopAsync(
        CancellationToken cancellationToken)
    {
        if (_udp is null)
            return;

        while (!cancellationToken.IsCancellationRequested)
        {
            UdpReceiveResult result;

            try
            {
                result = await _udp.ReceiveAsync(
                    cancellationToken);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (ObjectDisposedException)
            {
                break;
            }
            catch (SocketException)
            {
                if (cancellationToken.IsCancellationRequested)
                    break;

                continue;
            }

            ProcessMessage(
                result.Buffer,
                result.RemoteEndPoint.Address);
        }
    }

    private void ProcessMessage(
        byte[] data,
        IPAddress remoteAddress)
    {
        try
        {
            using JsonDocument document =
                JsonDocument.Parse(data);

            JsonElement root =
                document.RootElement;

            string type =
                root.TryGetProperty(
                    "type",
                    out var typeProperty)
                    ? typeProperty.GetString() ?? ""
                    : "";

            if (type == "device")
            {
                ProcessDevice(
                    root,
                    remoteAddress);

                return;
            }

            if (type == "ack")
            {
                ProcessAck(root);
            }
        }
        catch (JsonException)
        {
        }
    }

    private void ProcessDevice(
        JsonElement root,
        IPAddress remoteAddress)
    {
        int version =
            root.TryGetProperty(
                "version",
                out var versionProperty)
                ? versionProperty.GetInt32()
                : 0;

        string deviceId =
            root.TryGetProperty(
                "deviceId",
                out var idProperty)
                ? idProperty.GetString() ?? ""
                : "";

        if (version != ProtocolVersion ||
            string.IsNullOrWhiteSpace(deviceId))
        {
            return;
        }

        int teamId =
            root.TryGetProperty(
                "teamId",
                out var teamProperty)
                ? teamProperty.GetInt32()
                : 0;

        if (teamId < 0 ||
            teamId > MaxTeamId)
        {
            return;
        }

        int playerId =
            root.TryGetProperty(
                "playerId",
                out var playerProperty)
                ? playerProperty.GetInt32()
                : 0;

        if (playerId < 0 ||
            playerId > MaxPlayerId)
        {
            return;
        }

        string state =
            root.TryGetProperty(
                "state",
                out var stateProperty)
                ? stateProperty.GetString() ?? ""
                : "";

        if (_devices.TryGetValue(
                deviceId,
                out var existing))
        {
            existing.Update(
                remoteAddress.ToString(),
                teamId,
                playerId,
                state,
                version);

            DevicesChanged?.Invoke(this, EventArgs.Empty);
            return;
        }

        var device = new CombatLRDevice
        {
            DeviceId = deviceId,
            IpAddress = remoteAddress.ToString(),
            TeamId = teamId,
            PlayerId = playerId,
            State = state,
            ProtocolVersion = version,
            LastSeen = DateTimeOffset.UtcNow
        };

        if (_devices.TryAdd(
                device.DeviceId,
                device))
        {
            DevicesChanged?.Invoke(
                this,
                EventArgs.Empty);
        }
    }

    private void ProcessAck(
        JsonElement root)
    {
        int sequence =
            root.TryGetProperty(
                "sequence",
                out var sequenceProperty)
                ? sequenceProperty.GetInt32()
                : 0;

        bool success =
            root.TryGetProperty(
                "success",
                out var successProperty) &&
            successProperty.GetBoolean();

        string message =
            root.TryGetProperty(
                "message",
                out var messageProperty)
                ? messageProperty.GetString() ?? ""
                : "";

        if (_pendingCommands.TryGetValue(
                sequence,
                out var completion))
        {
            completion.TrySetResult(
                new CombatLRAck
                {
                    Sequence = sequence,
                    Success = success,
                    Message = message
                });
        }
    }

    public async ValueTask DisposeAsync()
    {
        await StopDiscoveryAsync();
    }
}

public sealed class DiscoveryMessage
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }
}

public sealed class SetTeamCommand
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }

    [JsonPropertyName("sequence")]
    public int Sequence { get; init; }

    [JsonPropertyName("teamId")]
    public int TeamId { get; init; }
}

public sealed class SetPlayerCommand
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }

    [JsonPropertyName("sequence")]
    public int Sequence { get; init; }

    [JsonPropertyName("playerId")]
    public int PlayerId { get; init; }
}

public sealed class StartGameCommand
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }

    [JsonPropertyName("sequence")]
    public int Sequence { get; init; }

    [JsonPropertyName("gameStart")]
    public string GameStart { get; init; } = "";
}

public sealed class StartGameResult
{
    public StartGameResult(
        string gameStart,
        IReadOnlyList<CombatLRDevice> successfulDevices,
        IReadOnlyList<CombatLRDevice> failedDevices)
    {
        GameStart = gameStart;
        SuccessfulDevices = successfulDevices;
        FailedDevices = failedDevices;
    }

    public string GameStart { get; }
    public IReadOnlyList<CombatLRDevice> SuccessfulDevices { get; }
    public IReadOnlyList<CombatLRDevice> FailedDevices { get; }

    public bool AllSucceeded =>
        FailedDevices.Count == 0 &&
        SuccessfulDevices.Count > 0;
}

public sealed class CombatLRAck
{
    public int Sequence { get; init; }
    public bool Success { get; init; }
    public string Message { get; init; } = "";
}

public sealed class CombatLRDevice : INotifyPropertyChanged
{
    private string _ipAddress = "";
    private int _teamId;
    private int _playerId;
    private bool _isDuplicatePlayerId;
    private string _state = "";
    private int _protocolVersion;
    private DateTimeOffset _lastSeen;

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Type { get; init; } = "";

    public int Version { get; init; }

    [JsonIgnore]
    public int ProtocolVersion
    {
        get => _protocolVersion;
        set => SetField(ref _protocolVersion, value);
    }

    public string DeviceId { get; init; } = "";

    [JsonIgnore]
    public string IpAddress
    {
        get => _ipAddress;
        set => SetField(ref _ipAddress, value);
    }

    public int TeamId
    {
        get => _teamId;
        set
        {
            if (SetField(ref _teamId, value))
            {
                OnPropertyChanged(nameof(TeamName));
                OnPropertyChanged(nameof(TeamBackgroundColor));
                OnPropertyChanged(nameof(TeamTextColor));
            }
        }
    }

    [JsonIgnore]
    public string TeamName =>
        TeamId switch
        {
            0 => "Red",
            1 => "Blue",
            2 => "Yellow",
            3 => "Green",
            _ => ""
        };

    public int PlayerId
    {
        get => _playerId;
        set => SetField(ref _playerId, value);
    }

    [JsonIgnore]
    public bool IsDuplicatePlayerId
    {
        get => _isDuplicatePlayerId;
        set
        {
            if (SetField(ref _isDuplicatePlayerId, value))
            {
                OnPropertyChanged(nameof(PlayerIdBackgroundColor));
                OnPropertyChanged(nameof(PlayerIdTextColor));
            }
        }
    }

    [JsonIgnore]
    public Color PlayerIdBackgroundColor =>
        IsDuplicatePlayerId
            ? Color.FromArgb("#FFCDD2")
            : Colors.White;

    [JsonIgnore]
    public Color PlayerIdTextColor =>
        IsDuplicatePlayerId
            ? Color.FromArgb("#B00020")
            : Colors.Black;

    public string State
    {
        get => _state;
        set => SetField(ref _state, value);
    }

    [JsonIgnore]
    public DateTimeOffset LastSeen
    {
        get => _lastSeen;
        set => SetField(ref _lastSeen, value);
    }

    [JsonIgnore]
    public Color TeamBackgroundColor =>
        TeamId switch
        {
            0 => Color.FromArgb("#FFCDD2"), // Red
            1 => Color.FromArgb("#BBDEFB"), // Blue
            2 => Color.FromArgb("#FFF9C4"), // Yellow
            3 => Color.FromArgb("#C8E6C9"), // Green
            _ => Colors.LightGray
        };

    [JsonIgnore]
    public Color TeamTextColor =>
        TeamId switch
        {
            0 => Color.FromArgb("#8B0000"), // Red
            1 => Color.FromArgb("#003C8F"), // Blue
            2 => Color.FromArgb("#5D5200"), // Yellow
            3 => Color.FromArgb("#1B5E20"), // Green
            _ => Colors.Black
        };

    public void Update(
        string ipAddress,
        int teamId,
        int playerId,
        string state,
        int protocolVersion)
    {
        IpAddress = ipAddress;
        TeamId = teamId;
        PlayerId = playerId;
        State = state;
        ProtocolVersion = protocolVersion;
        LastSeen = DateTimeOffset.UtcNow;
    }

    private bool SetField<T>(
        ref T field,
        T value,
        [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(
                field,
                value))
        {
            return false;
        }

        field = value;

        OnPropertyChanged(propertyName);

        return true;
    }

    private void OnPropertyChanged(
        string? propertyName)
    {
        PropertyChanged?.Invoke(
            this,
            new PropertyChangedEventArgs(propertyName));
    }
}
