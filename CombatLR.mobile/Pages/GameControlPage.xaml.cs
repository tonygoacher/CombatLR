using System.Collections.ObjectModel;
using CombatLR.mobile.Services;

namespace CombatLR.mobile.Pages;

public partial class GameControlPage : ContentPage
{
    private readonly CombatLRNetworkService _networkService = new();

    private readonly ObservableCollection<CombatLRDevice>
        _devices = new();

    public GameControlPage()
    {
        InitializeComponent();

        DevicesView.ItemsSource = _devices;

        _networkService.DevicesChanged +=
            NetworkService_DevicesChanged;
    }

    protected override async void OnAppearing()
    {
        base.OnAppearing();

        await StartDiscoveryAsync();
    }

    protected override async void OnDisappearing()
    {
        await _networkService.StopDiscoveryAsync();

        base.OnDisappearing();
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

    private void AddNewDevices()
    {
        foreach (var device in _networkService.Devices)
        {
            if (!_devices.Contains(device))
                _devices.Add(device);
        }

        UpdateDuplicatePlayerIds();

        SetControlButtonsEnabled(_devices.Count > 0);
    }

    private void SetControlButtonsEnabled(bool enabled)
    {
        FriendlyFireOnButton.IsEnabled = enabled;
        FriendlyFireOffButton.IsEnabled = enabled;
        StartGameButton.IsEnabled = enabled;
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
        if (_devices.Count == 0)
            return;

        SetControlButtonsEnabled(false);

        try
        {
            var results = await Task.WhenAll(
                _devices.Select(async device =>
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
        if (_devices.Count == 0)
        {
            return;
        }

        SetControlButtonsEnabled(false);
        DiscoverButton.IsEnabled = false;

        try
        {
            var result =
                await _networkService.StartGameAsync();

            if (result.SuccessfulDevices.Count == 0)
            {
                await DisplayAlert(
                    "Start Game",
                    "No CombatLR devices acknowledged the start command.",
                    "OK");

                return;
            }

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
