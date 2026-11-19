using CombatLR.mobile.Models;

namespace CombatLR.mobile.Pages;

public partial class PickupEditPage : ContentPage
{
    private readonly GamePickup _pickup;

    private readonly TaskCompletionSource<bool> _completion =
    new();

    public Task Completion =>
        _completion.Task;


    public PickupEditPage(
        GamePickup pickup)
    {
        InitializeComponent();

        _pickup = pickup;

        TitleLabel.Text =
            "Edit Pickup";

        TypePicker.SelectedIndex =
            (int)_pickup.Type;

        SpawnTimeEntry.Text =
            _pickup.SpawnTimeSeconds.ToString();

        LifespanEntry.Text =
            _pickup.LifespanSeconds.ToString();

        TypePicker.SelectedIndexChanged +=
            TypePicker_SelectedIndexChanged;

        UpdateSpawnTimeEditor();
    }

    private void TypePicker_SelectedIndexChanged(
        object? sender,
        EventArgs e)
    {
        UpdateSpawnTimeEditor();
    }

    private void UpdateSpawnTimeEditor()
    {
        var isAuto =
            TypePicker.SelectedIndex == (int)PickupType.Auto;

        SpawnTimeEntry.IsEnabled = !isAuto;
        SpawnTimeLabel.TextColor = isAuto ? Colors.Gray : Colors.White;
        SpawnTimeEntry.TextColor = isAuto ? Colors.Gray : Colors.White;
        SpawnTimeEntry.BackgroundColor = isAuto
            ? Color.FromArgb("#333333")
            : Color.FromArgb("#44000000");

        LifespanEntry.IsEnabled = !isAuto;
        LifespanLabel.TextColor = isAuto ? Colors.Gray : Colors.White;
        LifespanEntry.TextColor = isAuto ? Colors.Gray : Colors.White;
        LifespanEntry.BackgroundColor = isAuto
            ? Color.FromArgb("#333333")
            : Color.FromArgb("#44000000");

        if (isAuto)
        {
            SpawnTimeEntry.Text = "0";
            LifespanEntry.Text = "0";
        }
    }

    private static bool TryParseSeconds(
        string? text,
        out int result)
    {
        result = 0;

        if (!int.TryParse(text, out var seconds))
            return false;

        if (seconds < 0)
            return false;

        result = seconds;

        return true;
    }

    private async void Ok_Clicked(
        object? sender,
        EventArgs e)
    {
        if (TypePicker.SelectedIndex < 0)
        {
            await DisplayAlert(
                "Invalid Type",
                "Select a pickup type.",
                "OK");

            return;
        }

        var pickupType =
            (PickupType)TypePicker.SelectedIndex;

        var spawnTime = 0;

        if (pickupType != PickupType.Auto &&
            !TryParseSeconds(
                SpawnTimeEntry.Text,
                out spawnTime))
        {
            await DisplayAlert(
                "Invalid Spawn Time",
                "Enter a whole number of seconds.",
                "OK");

            return;
        }

        var lifespan = 0;

        if (pickupType != PickupType.Auto &&
            !TryParseSeconds(
                LifespanEntry.Text,
                out lifespan))
        {
            await DisplayAlert(
                "Invalid Lifespan",
                "Enter a whole number of seconds.",
                "OK");

            return;
        }

        _pickup.Type =
            pickupType;

        // Auto pickups are allocated by the game engine and have
        // neither an individual spawn time nor a lifespan.
        _pickup.SpawnTimeSeconds =
            pickupType == PickupType.Auto ? 0 : spawnTime;

        _pickup.LifespanSeconds =
            pickupType == PickupType.Auto ? 0 : lifespan;

        _completion.TrySetResult(true);
        await Navigation.PopModalAsync();
    }

    private async void Cancel_Clicked(
        object? sender,
        EventArgs e)
    {
        _completion.TrySetResult(false);
        await Navigation.PopModalAsync();
    }
}
