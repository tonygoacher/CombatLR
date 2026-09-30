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
