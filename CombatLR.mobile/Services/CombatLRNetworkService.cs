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

public sealed class FtpTransferProgress
{
    public string FileName { get; init; } = "";
    public long BytesTransferred { get; init; }
    public long TotalBytes { get; init; }

    public int Percentage =>
        TotalBytes <= 0
            ? 0
            : Math.Min(100, (int)(BytesTransferred * 100L / TotalBytes));
}

public sealed class CombatLRNetworkService : IAsyncDisposable
{
    public const int DiscoveryPort = 4210;
    public const int ProtocolVersion = 1;
    public const int MaxTeamId = 3;
    public const int MaxPlayerId = 127;
    public const int FtpPort = 21;

    private const int DiscoveryIntervalMilliseconds = 2000;
    private const int DiscoveryMissLimit = 5;
    private const int CommandTimeoutMilliseconds = 2000;
    private const int CommandMaxAttempts = 3;
    private const int CommandRetryDelayMilliseconds = 100;

    private readonly ConcurrentDictionary<string, CombatLRDevice> _devices = new();

    private UdpClient? _udp;
    private CancellationTokenSource? _cts;
    private Task? _discoveryTask;
    private int _nextSequence;

    public IReadOnlyCollection<CombatLRDevice> Devices =>
        _devices.Values.ToArray();

    public event EventHandler? DevicesChanged;

    public event EventHandler<CombatLRDevice>? StartGameAcknowledged;

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
        if (teamId < 0 ||
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

        CombatLRAck? ack =
            await SendTcpCommandAsync(
                device,
                command,
                sequence);

        if (ack is null ||
            !ack.Success)
        {
            return false;
        }

        device.TeamId = teamId;

        return true;
    }

    public async Task<bool> SetPlayerIdAsync(
        CombatLRDevice device,
        int playerId)
    {
        if (playerId < 0 ||
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

        CombatLRAck? ack =
            await SendTcpCommandAsync(
                device,
                command,
                sequence);

        if (ack is null ||
            !ack.Success)
        {
            return false;
        }

        device.PlayerId = playerId;

        return true;
    }

    public async Task<bool> SetFriendlyFireAsync(
        CombatLRDevice device,
        bool friendlyFire)
    {
        int sequence =
            Interlocked.Increment(ref _nextSequence);

        var command = new SetFriendlyFireCommand
        {
            Type = "setFriendlyFire",
            Version = ProtocolVersion,
            Sequence = sequence,
            FriendlyFire = friendlyFire
        };

        CombatLRAck? ack =
            await SendTcpCommandAsync(
                device,
                command,
                sequence);

        if (ack is null ||
            !ack.Success)
        {
            return false;
        }

        device.FriendlyFire = friendlyFire;

        return true;
    }

    public Task<StartGameResult> StartGameAsync()
    {
        string gameStart =
            DateTime.UtcNow.ToString(
                "HH:mm:ss",
                CultureInfo.InvariantCulture);

        return StartGameAsync(
            gameStart,
            _devices.Values.ToArray());
    }

    public async Task<StartGameResult> StartGameAsync(
        string gameStart,
        IReadOnlyCollection<CombatLRDevice> devices)
    {
        if (_udp is null)
        {
            return new StartGameResult(
                gameStart,
                Array.Empty<CombatLRDevice>(),
                devices.ToArray());
        }

        if (devices.Count == 0)
        {
            return new StartGameResult(
                gameStart,
                Array.Empty<CombatLRDevice>(),
                Array.Empty<CombatLRDevice>());
        }

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
        int sequence =
            Interlocked.Increment(ref _nextSequence);

        var command = new StartGameCommand
        {
            Type = "startGame",
            Version = ProtocolVersion,
            Sequence = sequence,
            GameStart = gameStart
        };

        CombatLRAck? ack =
            await SendTcpCommandAsync(
                device,
                command,
                sequence);

        if (ack is null ||
            !ack.Success)
        {
            return false;
        }

        StartGameAcknowledged?.Invoke(
            this,
            device);

        return true;
    }

    private async Task<CombatLRAck?> SendTcpCommandAsync<TCommand>(
        CombatLRDevice device,
        TCommand command,
        int expectedSequence)
    {
        if (string.IsNullOrWhiteSpace(device.IpAddress))
            return null;

        // Retry the complete request/ACK exchange, not just connection setup.
        // The same command and sequence are reused for every attempt.
        for (int attempt = 1;
             attempt <= CommandMaxAttempts;
             attempt++)
        {
            async Task<bool> RetryAsync(string reason)
            {
                if (attempt >= CommandMaxAttempts)
                {
                    Console.WriteLine(
                        $"CombatLR TCP: {reason} for {device.DeviceId}; " +
                        "no attempts remaining.");
                    return false;
                }

                Console.WriteLine(
                    $"CombatLR TCP: {reason} for {device.DeviceId}; " +
                    $"retrying ({attempt + 1}/{CommandMaxAttempts})...");

                await Task.Delay(CommandRetryDelayMilliseconds);
                return true;
            }

            try
            {
                Console.WriteLine(
                    $"CombatLR TCP: attempt {attempt}/{CommandMaxAttempts} " +
                    $"connecting to {device.DeviceId} " +
                    $"at {device.IpAddress}:4211");

                using var client = new TcpClient();

                using var connectTimeout =
                    new CancellationTokenSource(
                        CommandTimeoutMilliseconds);

                await client.ConnectAsync(
                    IPAddress.Parse(device.IpAddress),
                    4211,
                    connectTimeout.Token);

                Console.WriteLine(
                    $"CombatLR TCP: connected to {device.DeviceId}");

                using var stream = client.GetStream();

                string json =
                    JsonSerializer.Serialize(command);

                byte[] data =
                    Encoding.UTF8.GetBytes(
                        json + "\n");

                using var ioTimeout =
                    new CancellationTokenSource(
                        CommandTimeoutMilliseconds);

                Console.WriteLine(
                    $"CombatLR TCP: sending to {device.DeviceId}: {json}");

                await stream.WriteAsync(
                    data,
                    ioTimeout.Token);

                await stream.FlushAsync(
                    ioTimeout.Token);

                Console.WriteLine(
                    $"CombatLR TCP: waiting for ACK from {device.DeviceId}");

                using var reader =
                    new StreamReader(
                        stream,
                        Encoding.UTF8,
                        leaveOpen: true);

                string? response =
                    await reader.ReadLineAsync(
                        ioTimeout.Token);

                if (string.IsNullOrWhiteSpace(response))
                {
                    if (await RetryAsync("empty ACK / connection closed"))
                        continue;

                    return null;
                }

                Console.WriteLine(
                    $"CombatLR TCP: received from {device.DeviceId}: {response}");

                var ack =
                    JsonSerializer.Deserialize<CombatLRAckEnvelope>(
                        response);

                if (ack is null)
                    return null;

                if (ack.Sequence != expectedSequence)
                {
                    Console.WriteLine(
                        $"CombatLR TCP: ACK sequence mismatch from " +
                        $"{device.DeviceId}: expected {expectedSequence}, " +
                        $"received {ack.Sequence}");
                    return null;
                }

                return new CombatLRAck
                {
                    Sequence = ack.Sequence,
                    Success = ack.Success,
                    Message = ack.Message
                };
            }
            catch (OperationCanceledException ex)
            {
                if (await RetryAsync($"timeout/cancel ({ex.Message})"))
                    continue;

                return null;
            }
            catch (SocketException ex)
            {
                if (await RetryAsync(
                        $"socket error {ex.SocketErrorCode} ({ex.Message})"))
                {
                    continue;
                }

                return null;
            }
            catch (IOException ex)
            {
                if (await RetryAsync($"I/O error ({ex.Message})"))
                    continue;

                return null;
            }
            catch (FormatException ex)
            {
                Console.WriteLine(
                    $"CombatLR TCP: BAD IP/FORMAT for " +
                    $"{device.DeviceId}: {ex.Message}");
                return null;
            }
            catch (JsonException ex)
            {
                Console.WriteLine(
                    $"CombatLR TCP: BAD ACK JSON from " +
                    $"{device.DeviceId}: {ex.Message}");
                return null;
            }
        }

        return null;
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

                CheckOfflineDevices();
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

        bool friendlyFire =
            root.TryGetProperty(
                "friendlyFire",
                out var friendlyFireProperty) &&
            friendlyFireProperty.ValueKind == JsonValueKind.True;

        bool ftpMode =
            root.TryGetProperty(
                "ftpMode",
                out var ftpModeProperty) &&
            ftpModeProperty.ValueKind == JsonValueKind.True;

        int planVersion =
            root.TryGetProperty(
                "planVersion",
                out var planVersionProperty)
                ? planVersionProperty.GetInt32()
                : 0;

        string planName =
            root.TryGetProperty(
                "planName",
                out var planNameProperty)
                ? planNameProperty.GetString() ?? ""
                : "";

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
                friendlyFire,
                ftpMode,
                state,
                version,
                planVersion,
                planName);

            existing.LastSeen = DateTimeOffset.UtcNow;
            existing.IsOnline = true;

            DevicesChanged?.Invoke(this, EventArgs.Empty);
            return;
        }

        var device = new CombatLRDevice
        {
            DeviceId = deviceId,
            IpAddress = remoteAddress.ToString(),
            TeamId = teamId,
            PlayerId = playerId,
            FriendlyFire = friendlyFire,
            FtpMode = ftpMode,
            State = state,
            ProtocolVersion = version,
            PlanVersion = planVersion,
            PlanName = planName,
            LastSeen = DateTimeOffset.UtcNow,
            IsOnline = true
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

    private void CheckOfflineDevices()
    {
        DateTimeOffset cutoff =
            DateTimeOffset.UtcNow.AddMilliseconds(
                -(DiscoveryIntervalMilliseconds * DiscoveryMissLimit));

        foreach (var device in _devices.Values)
        {
            if (device.IsOnline &&
                device.LastSeen <= cutoff)
            {
                device.IsOnline = false;
            }
        }
    }

#pragma warning disable SYSLIB0014
    public async Task<bool> UploadProjectFileAsync(
        CombatLRDevice device,
        string localFilePath,
        string remoteFileName,
        string username,
        string password,
        CancellationToken cancellationToken = default,
        IProgress<FtpTransferProgress>? progress = null)
    {
        if (!device.IsOnline ||
            !device.FtpMode ||
            string.IsNullOrWhiteSpace(device.IpAddress) ||
            !File.Exists(localFilePath) ||
            string.IsNullOrWhiteSpace(username))
        {
            return false;
        }

        FtpWebRequest? request = null;
        long totalBytes = 0;
        long bytesTransferred = 0;

        try
        {
            request =
                (FtpWebRequest)WebRequest.Create(
                    new Uri($"ftp://{device.IpAddress}:{FtpPort}/{Uri.EscapeDataString(remoteFileName)}"));

            request.Method = WebRequestMethods.Ftp.UploadFile;
            request.Credentials = new NetworkCredential(username, password);
            request.UseBinary = true;
            request.KeepAlive = false;
            request.Timeout = 10000;
            request.ReadWriteTimeout = 10000;

            var fileInfo = new FileInfo(localFilePath);
            totalBytes = fileInfo.Length;
            request.ContentLength = totalBytes;

            await using FileStream fileStream =
                new FileStream(
                    localFilePath,
                    FileMode.Open,
                    FileAccess.Read,
                    FileShare.Read,
                    bufferSize: 16 * 1024,
                    useAsync: true);

            byte[] buffer = new byte[16 * 1024];

            progress?.Report(new FtpTransferProgress
            {
                FileName = remoteFileName,
                BytesTransferred = 0,
                TotalBytes = totalBytes
            });

            // Send the complete file. Reaching 100% means all bytes have
            // successfully been accepted by the FTP request stream.
            await using (Stream requestStream =
                await request.GetRequestStreamAsync())
            {
                int bytesRead;

                while ((bytesRead = await fileStream.ReadAsync(
                           buffer.AsMemory(0, buffer.Length),
                           cancellationToken)) > 0)
                {
                    await requestStream.WriteAsync(
                        buffer.AsMemory(0, bytesRead),
                        cancellationToken);

                    bytesTransferred += bytesRead;

                    progress?.Report(new FtpTransferProgress
                    {
                        FileName = remoteFileName,
                        BytesTransferred = bytesTransferred,
                        TotalBytes = totalBytes
                    });
                }
            }

            // The data stream is now closed. Normally the FTP server returns
            // a 226 response here. SimpleFTPServer can, however, close the
            // control connection without FtpWebRequest receiving that final
            // response. Do not turn a successfully transferred file into a
            // failed update just because that final acknowledgement is lost.
            try
            {
                using var response =
                    (FtpWebResponse)await request.GetResponseAsync();

                Console.WriteLine(
                    $"CombatLR FTP: {remoteFileName} -> {device.DeviceId}: " +
                    $"{response.StatusCode} {response.StatusDescription?.Trim()}");

                return (int)response.StatusCode >= 200 &&
                       (int)response.StatusCode < 300;
            }
            catch (WebException ex) when
                (bytesTransferred == totalBytes &&
                 ex.Response is null)
            {
                // All file bytes were written successfully, but no FTP
                // response was received. Treat the transfer as successful.
                // This handles the SimpleFTPServer/FtpWebRequest completion
                // behaviour without making the user retry an uploaded file.
                Console.WriteLine(
                    $"CombatLR FTP: {remoteFileName} -> {device.DeviceId}: " +
                    "transfer complete; no final FTP response received.");

                return true;
            }
        }
        catch (WebException ex)
        {
            if (ex.Response is FtpWebResponse ftpResponse)
            {
                Console.WriteLine(
                    $"CombatLR FTP: upload of {remoteFileName} to " +
                    $"{device.DeviceId} failed: " +
                    $"{ftpResponse.StatusCode} {ftpResponse.StatusDescription?.Trim()}");

                ftpResponse.Dispose();
            }
            else
            {
                Console.WriteLine(
                    $"CombatLR FTP: upload of {remoteFileName} to " +
                    $"{device.DeviceId} failed: {ex.Message}");
            }

            return false;
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"CombatLR FTP: upload of {remoteFileName} to " +
                $"{device.DeviceId} failed: {ex.Message}");
            return false;
        }
    }

#pragma warning restore SYSLIB0014

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

public sealed class SetFriendlyFireCommand
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }

    [JsonPropertyName("sequence")]
    public int Sequence { get; init; }

    [JsonPropertyName("friendlyFire")]
    public bool FriendlyFire { get; init; }
}

internal sealed class CombatLRAckEnvelope
{
    [JsonPropertyName("type")]
    public string Type { get; init; } = "";

    [JsonPropertyName("version")]
    public int Version { get; init; }

    [JsonPropertyName("sequence")]
    public int Sequence { get; init; }

    [JsonPropertyName("success")]
    public bool Success { get; init; }

    [JsonPropertyName("message")]
    public string Message { get; init; } = "";
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
    private bool _friendlyFire;
    private bool _ftpMode;
    private bool _isDuplicatePlayerId;
    private string _state = "";
    private int _protocolVersion;
    private int _planVersion;
    private string _planName = "";
    private DateTimeOffset _lastSeen;
    private bool _isOnline = true;

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Type { get; init; } = "";

    public int Version { get; init; }

    [JsonIgnore]
    public int ProtocolVersion
    {
        get => _protocolVersion;
        set => SetField(ref _protocolVersion, value);
    }

    public int PlanVersion
    {
        get => _planVersion;
        set => SetField(ref _planVersion, value);
    }

    public string PlanName
    {
        get => _planName;
        set => SetField(ref _planName, value);
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

    public bool FriendlyFire
    {
        get => _friendlyFire;
        set
        {
            if (SetField(ref _friendlyFire, value))
                OnPropertyChanged(nameof(FriendlyFireIcon));
        }
    }

    [JsonIgnore]
    public string FriendlyFireIcon =>
        FriendlyFire
            ? "friendly_fire_on.png"
            : "friendly_fire_off.png";

    public bool FtpMode
    {
        get => _ftpMode;
        set => SetField(ref _ftpMode, value);
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
        set
        {
            if (SetField(ref _state, value))
                OnPropertyChanged(nameof(DisplayState));
        }
    }

    [JsonIgnore]
    public DateTimeOffset LastSeen
    {
        get => _lastSeen;
        set => SetField(ref _lastSeen, value);
    }

    [JsonIgnore]
    public bool IsOnline
    {
        get => _isOnline;
        set
        {
            if (SetField(ref _isOnline, value))
            {
                OnPropertyChanged(nameof(TeamBackgroundColor));
                OnPropertyChanged(nameof(TeamTextColor));
                OnPropertyChanged(nameof(FriendlyFireButtonEnabled));
                OnPropertyChanged(nameof(FriendlyFireButtonOpacity));
                OnPropertyChanged(nameof(DisplayState));
            }
        }
    }

    [JsonIgnore]
    public bool FriendlyFireButtonEnabled =>
        IsOnline;

    [JsonIgnore]
    public double FriendlyFireButtonOpacity =>
        IsOnline ? 1.0 : 0.35;

    [JsonIgnore]
    public string DisplayState =>
        IsOnline ? State : "OFFLINE";

    [JsonIgnore]
    public Color TeamBackgroundColor =>
        !IsOnline
            ? Color.FromArgb("#E0E0E0")
            : TeamId switch
        {
            0 => Color.FromArgb("#FFCDD2"), // Red
            1 => Color.FromArgb("#BBDEFB"), // Blue
            2 => Color.FromArgb("#FFF9C4"), // Yellow
            3 => Color.FromArgb("#C8E6C9"), // Green
            _ => Colors.LightGray
        };

    [JsonIgnore]
    public Color TeamTextColor =>
        !IsOnline
            ? Color.FromArgb("#777777")
            : TeamId switch
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
        bool friendlyFire,
        bool ftpMode,
        string state,
        int protocolVersion,
        int planVersion = 0,
        string planName = "")
    {
        IpAddress = ipAddress;
        TeamId = teamId;
        PlayerId = playerId;
        FriendlyFire = friendlyFire;
        FtpMode = ftpMode;
        State = state;
        ProtocolVersion = protocolVersion;
        PlanVersion = planVersion;
        PlanName = planName;
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
