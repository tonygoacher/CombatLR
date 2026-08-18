using CombatLR.mobile.Models;

namespace CombatLR.mobile.Pages;

public partial class ZoneEditPage : ContentPage
{
    private readonly GameZone _zone;
    private readonly TaskCompletionSource<bool>
    _completionSource = new();


    public ZoneEditPage(
        GameZone zone,
        int zoneNumber)
    {
        InitializeComponent();

        _zone = zone;

        TitleLabel.Text =
            $"Edit Zone {zoneNumber}";

        RadiusEntry.Text =
            zone.RadiusMetres.ToString("0.0");
        StartTimeEntry.Text = zone.StartTime.ToString();

        TransitionTimeEntry.Text =
          zone.TransitionTime.ToString(); ;


        // Zone 1 has no previous zone,
        // so transition time has no meaning.
        if (zoneNumber == 1)
        {
            TransitionTimeEntry.IsEnabled = false;
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
        if (!double.TryParse(
                RadiusEntry.Text,
                out var radius) ||
            radius <= 0)
        {
            await DisplayAlert(
                "Invalid Radius",
                "Radius must be greater than zero.",
                "OK");

            return;
        }

        if (!TryParseSeconds(
                StartTimeEntry.Text,
                out var startTime))
        {
            await DisplayAlert(
                "Invalid Start Time",
                "Enter time in seconds",
                "OK");

            return;
        }

        _zone.RadiusMetres =
            radius;

        _zone.StartTime =
            startTime;

        if (TransitionTimeEntry.IsEnabled)
        {
            if (!TryParseSeconds(
                    TransitionTimeEntry.Text,
                    out var transitionTime))
            {
                await DisplayAlert(
                    "Invalid Transition Time",
                    "Enter time in seconds",
                    "OK");

                return;
            }

            _zone.TransitionTime =
                transitionTime;
        }

        _completionSource.TrySetResult(true);
        await Navigation.PopModalAsync();
    }

    private async void Cancel_Clicked(
        object? sender,
        EventArgs e)
    {
        _completionSource.TrySetResult(false);
        await Navigation.PopModalAsync();
    }

    public Task<bool> Completion =>
    _completionSource.Task;
}