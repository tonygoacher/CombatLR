using System.Collections.ObjectModel;
using System.Globalization;
using CombatLR.mobile.Services;

namespace CombatLR.mobile.Pages;

public partial class GameControlPage : ContentPage
{
    private readonly CombatLRNetworkService _networkService = new();
    private readonly SelectedGamePlanService _selectedGamePlan;

    private readonly ObservableCollection<CombatLRDevice>
        _devices = new();

    private string? _storedGameStartUtc;

    public GameControlPage(SelectedGamePlanService selectedGamePlan)
    {
        InitializeComponent();
        _selectedGamePlan = selectedGamePlan;

        DevicesView.ItemsSource = _devices;

        _networkService.DevicesChanged +=
            NetworkService_DevicesChanged;

        LoadStoredGameStart();
        UpdateDeviceCount();
    }

    protected override async void OnAppearing()
    {
        base.OnAppearing();

        _networkService.StartGameAcknowledged +=
            NetworkService_StartGameAcknowledged;

        LoadStoredGameStart();
        UpdateSelectedGamePlanLabel();

        await StartDiscoveryAsync();
    }

    protected override async void OnDisappearing()
    {
        base.OnDisappearing();

        _networkService.StartGameAcknowledged -=
            NetworkService_StartGameAcknowledged;

        try
        {
            await _networkService.StopDiscoveryAsync();
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"Game Control: discovery shutdown failed: {ex.Message}");
        }
    }

    private async void DiscoverButton_Clicked(
        object? sender,
        EventArgs e)
    {
        _networkService.ClearDevices();
        _devices.Clear();

        SetControlButtonsEnabled(false);

        await StartDiscoveryAsync();
    }

    private async Task StartDiscoveryAsync()
    {
        DiscoverButton.IsEnabled = false;
        DiscoveryActivity.IsVisible = true;
        DiscoveryActivity.IsRunning = true;

        try
        {
            await _networkService.StartDiscoveryAsync();
        }
        catch (Exception ex)
        {
            await DisplayAlert(
                "Discovery",
                ex.Message,
                "OK");
        }
        finally
        {
            DiscoverButton.IsEnabled = true;
            DiscoveryActivity.IsVisible = false;
            DiscoveryActivity.IsRunning = false;

            SetControlButtonsEnabled(_devices.Count > 0);
        }
    }

    private void NetworkService_DevicesChanged(
        object? sender,
        EventArgs e)
    {
        MainThread.BeginInvokeOnMainThread(
            AddNewDevices);
    }

    private void NetworkService_StartGameAcknowledged(
        object? sender,
        CombatLRDevice device)
    {
        MainThread.BeginInvokeOnMainThread(() =>
        {
            var existing = _devices.FirstOrDefault(
                d => d.DeviceId == device.DeviceId);

            if (existing is not null)
            {
                existing.PropertyChanged -= Device_PropertyChanged;
                _devices.Remove(existing);
            }

            UpdateDuplicatePlayerIds();
            UpdateDeviceCount();
            SetControlButtonsEnabled(_devices.Count > 0);
        });
    }

    private void AddNewDevices()
    {
        foreach (var device in _networkService.Devices)
        {
            if (!_devices.Contains(device))
            {
                _devices.Add(device);
                device.PropertyChanged += Device_PropertyChanged;
            }

            UpdateDevicePlanStatus(device);
        }

        UpdateDuplicatePlayerIds();
        UpdateDeviceCount();

        SetControlButtonsEnabled(_devices.Count > 0);
    }

    private void Device_PropertyChanged(
        object? sender,
        System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName is nameof(CombatLRDevice.IsOnline) or nameof(CombatLRDevice.FtpMode))
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                UpdateDeviceCount();
                SetControlButtonsEnabled(HasAvailablePlayers);
            });
        }
    }

    private void UpdateSelectedGamePlanLabel()
    {
        SelectedGamePlanLabel.Text = _selectedGamePlan.GamePlan is null
            ? "No game plan selected"
            : $"PLAN: {_selectedGamePlan.ProjectName}  v{_selectedGamePlan.GamePlan.Version}";

        foreach (var device in _devices)
            UpdateDevicePlanStatus(device);
    }

    private void UpdateDevicePlanStatus(CombatLRDevice device)
    {
        device.SetCurrentGamePlan(
            _selectedGamePlan.ProjectName,
            _selectedGamePlan.GamePlan?.Version ?? -1);
    }

    private void UpdateDeviceCount()
    {
        int onlineCount = _devices.Count(device => device.IsOnline);
        DevicesHeaderLabel.Text = $"COMBATLR DEVICES: {onlineCount}";
    }

    private bool HasAvailablePlayers =>
        _devices.Any(device => device.IsOnline && !device.FtpMode);

    private void SetControlButtonsEnabled(bool enabled)
    {
        enabled &= HasAvailablePlayers;

        FriendlyFireOnButton.IsEnabled = enabled;
        FriendlyFireOffButton.IsEnabled = enabled;
        StartGameButton.IsEnabled = enabled;
        LateStartButton.IsEnabled =
            enabled &&
            !string.IsNullOrWhiteSpace(_storedGameStartUtc);
    }

    private void LoadStoredGameStart()
    {
        var session = new GameSessionService().GetSession(
            _selectedGamePlan.ProjectName,
            _selectedGamePlan.GamePlan?.Version ?? -1);

        _storedGameStartUtc = session?.StartedAtUtc.UtcDateTime.ToString(
            "HH:mm:ss",
            CultureInfo.InvariantCulture);

        if (session is null)
        {
            GameStartLabel.Text = "GAME START: Not set for this plan";
        }
        else
        {
            GameStartLabel.Text =
                $"GAME START: {FormatGameStart(_storedGameStartUtc!)}";
        }

        SetControlButtonsEnabled(_devices.Count > 0);
    }

    private static string FormatGameStart(string utcGameStart)
    {
        if (!DateTime.TryParseExact(
                utcGameStart,
                "HH:mm:ss",
                CultureInfo.InvariantCulture,
                DateTimeStyles.AssumeUniversal |
                DateTimeStyles.AdjustToUniversal,
                out DateTime utc))
        {
            return utcGameStart;
        }

        string local =
            utc.ToLocalTime().ToString(
                "HH:mm:ss",
                CultureInfo.InvariantCulture);

        string utcText =
            utc.ToString(
                "HH:mm:ss",
                CultureInfo.InvariantCulture);

        return $"{local} ({utcText} UTC)";
    }

    private void UpdateDuplicatePlayerIds()
    {
        var duplicateIds = _devices
            .GroupBy(device => device.PlayerId)
            .Where(group => group.Count() > 1)
            .Select(group => group.Key)
            .ToHashSet();

        foreach (var device in _devices)
            device.IsDuplicatePlayerId = duplicateIds.Contains(device.PlayerId);
    }

    private async void FriendlyFireOnButton_Clicked(
        object? sender,
        EventArgs e)
    {
        await SetFriendlyFireAsync(true);
    }

    private async void FriendlyFireOffButton_Clicked(
        object? sender,
        EventArgs e)
    {
        await SetFriendlyFireAsync(false);
    }

    private async void FriendlyFireIcon_Tapped(
        object? sender,
        TappedEventArgs e)
    {
        if (sender is not Image image ||
            image.BindingContext is not CombatLRDevice device)
        {
            return;
        }

        if (!device.IsOnline)
            return;

        bool newState = !device.FriendlyFire;

        // Prevent double taps while the command is in flight.
        image.IsEnabled = false;

        try
        {
            bool success =
                await _networkService.SetFriendlyFireAsync(
                    device,
                    newState);

            if (!success)
            {
                string action =
                    newState
                        ? "enable"
                        : "disable";

                await DisplayAlert(
                    "Friendly Fire",
                    $"Could not {action} friendly fire on {device.DeviceId}.",
                    "OK");
            }
        }
        finally
        {
            image.IsEnabled = true;
        }
    }

    private async Task SetFriendlyFireAsync(bool enabled)
    {
        var availableDevices = _devices
            .Where(device => device.IsOnline && !device.FtpMode)
            .ToArray();

        if (availableDevices.Length == 0)
            return;

        SetControlButtonsEnabled(false);

        try
        {
            var results = await Task.WhenAll(
                availableDevices.Select(async device =>
                    new
                    {
                        Device = device,
                        Success =
                            await _networkService.SetFriendlyFireAsync(
                                device,
                                enabled)
                    }));

            var failed = results
                .Where(result => !result.Success)
                .Select(result => result.Device.DeviceId)
                .ToArray();

            if (failed.Length > 0)
            {
                string action =
                    enabled
                        ? "enable"
                        : "disable";

                await DisplayAlert(
                    "Friendly Fire",
                    $"Could not {action} friendly fire on: " +
                    string.Join(", ", failed),
                    "OK");
            }
        }
        finally
        {
            SetControlButtonsEnabled(_devices.Count > 0);
        }
    }

    private async void StartGameButton_Clicked(
        object? sender,
        EventArgs e)
    {
        var availableDevices = _devices
            .Where(device => device.IsOnline && !device.FtpMode)
            .ToArray();

        if (availableDevices.Length == 0)
            return;

        SetControlButtonsEnabled(false);
        DiscoverButton.IsEnabled = false;

        try
        {
            var result =
                await _networkService.StartGameAsync(
                    DateTime.UtcNow.ToString("HH:mm:ss", CultureInfo.InvariantCulture),
                    availableDevices);

            if (result.SuccessfulDevices.Count == 0)
            {
                await DisplayAlert(
                    "Start Game",
                    "No CombatLR devices acknowledged the start command.",
                    "OK");

                return;
            }

            var session = new GameSessionService().RecordStart(
                _selectedGamePlan.ProjectName ?? string.Empty,
                _selectedGamePlan.GamePlan?.Version ?? -1,
                result.GameStart);

            _storedGameStartUtc = session.StartedAtUtc.UtcDateTime.ToString(
                "HH:mm:ss",
                CultureInfo.InvariantCulture);

            GameStartLabel.Text =
                $"GAME START: {FormatGameStart(_storedGameStartUtc)}";

            if (result.FailedDevices.Count == 0)
            {
                return;
            }

            string failed =
                string.Join(
                    ", ",
                    result.FailedDevices.Select(
                        device => device.DeviceId));

            await DisplayAlert(
                "Start Game",
                $"Game start {result.GameStart} was acknowledged by " +
                $"{result.SuccessfulDevices.Count} device(s), but failed on: {failed}.",
                "OK");
        }
        finally
        {
            DiscoverButton.IsEnabled = true;
            SetControlButtonsEnabled(_devices.Count > 0);
        }
    }

    private async void LateStartButton_Clicked(
        object? sender,
        EventArgs e)
    {
        var availableDevices = _devices
            .Where(device => device.IsOnline && !device.FtpMode)
            .ToArray();

        if (string.IsNullOrWhiteSpace(_storedGameStartUtc) ||
            availableDevices.Length == 0)
        {
            return;
        }

        SetControlButtonsEnabled(false);
        DiscoverButton.IsEnabled = false;

        try
        {
            var result =
                await _networkService.StartGameAsync(
                    _storedGameStartUtc,
                    availableDevices);

            if (result.SuccessfulDevices.Count == 0)
            {
                await DisplayAlert(
                    "Late Start",
                    "No CombatLR devices acknowledged the late start command.",
                    "OK");

                return;
            }

            if (result.FailedDevices.Count == 0)
                return;

            string failed =
                string.Join(
                    ", ",
                    result.FailedDevices.Select(
                        device => device.DeviceId));

            await DisplayAlert(
                "Late Start",
                $"Game start {_storedGameStartUtc} was acknowledged by " +
                $"{result.SuccessfulDevices.Count} device(s), but failed on: {failed}.",
                "OK");
        }
        finally
        {
            DiscoverButton.IsEnabled = true;
            SetControlButtonsEnabled(_devices.Count > 0);
        }
    }

    private async void PlayerIdEntry_Completed(
        object? sender,
        EventArgs e)
    {
        if (sender is not Entry entry ||
            entry.BindingContext is not CombatLRDevice device)
        {
            return;
        }

        if (!int.TryParse(entry.Text, out int playerId) ||
            playerId < 0 ||
            playerId > CombatLRNetworkService.MaxPlayerId)
        {
            entry.Text = device.PlayerId.ToString();
            return;
        }

        if (playerId == device.PlayerId)
            return;

        entry.IsEnabled = false;

        try
        {
            bool success =
                await _networkService.SetPlayerIdAsync(
                    device,
                    playerId);

            if (!success)
            {
                await DisplayAlert(
                    "Player ID",
                    $"Could not set Player ID {playerId} on {device.DeviceId}.",
                    "OK");

                entry.Text = device.PlayerId.ToString();
            }

            UpdateDuplicatePlayerIds();
        }
        finally
        {
            entry.IsEnabled = true;
        }
    }

    private async void TeamPicker_SelectedIndexChanged(
        object? sender,
        EventArgs e)
    {
        if (sender is not Picker picker ||
            picker.BindingContext is not CombatLRDevice device)
        {
            return;
        }

        if (picker.SelectedItem is not string teamName)
            return;

        int teamId = teamName switch
        {
            "Red" => 0,
            "Blue" => 1,
            "Yellow" => 2,
            "Green" => 3,
            _ => -1
        };

        if (teamId < 0 ||
            teamId > CombatLRNetworkService.MaxTeamId)
        {
            return;
        }

        // Ignore picker changes which simply reflect
        // the current authoritative TeamId.
        if (teamId == device.TeamId)
            return;

        picker.IsEnabled = false;

        try
        {
            bool success =
                await _networkService.SetTeamIdAsync(
                    device,
                    teamId);

            if (!success)
            {
                await DisplayAlert(
                    "Team",
                    $"Could not set Team {teamId} on {device.DeviceId}.",
                    "OK");

                // TeamId is authoritative.
                // Restore the picker from the device state.
                picker.SelectedItem = device.TeamName;
            }
        }
        finally
        {
            picker.IsEnabled = true;
        }
    }
}
