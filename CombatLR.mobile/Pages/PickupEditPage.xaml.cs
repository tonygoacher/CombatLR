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

        TypePicker.SelectedIndexChanged +=
            TypePicker_SelectedIndexChanged;

        UpdateSpawnTimeEditor();

        LifespanEntry.Text =
            _pickup.LifespanSeconds.ToString();
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

        if (isAuto)
            SpawnTimeEntry.Text = "0";
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

        if (!TryParseSeconds(
                LifespanEntry.Text,
                out var lifespan))
        {
            await DisplayAlert(
                "Invalid Lifespan",
                "Enter a whole number of seconds.",
                "OK");

            return;
        }

        _pickup.Type =
            pickupType;

        // Auto pickups are spawned by the game engine and never have
        // an individual spawn time.
        _pickup.SpawnTimeSeconds =
            pickupType == PickupType.Auto ? 0 : spawnTime;

        _pickup.LifespanSeconds =
            lifespan;

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
