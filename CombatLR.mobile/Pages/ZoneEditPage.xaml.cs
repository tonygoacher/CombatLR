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
        // The editor uses whole minutes; GamePlan/JSON remain seconds.
        StartTimeEntry.Text = Math.Max(0, (int)Math.Round(zone.StartTime / 60.0)).ToString();
        LifespanEntry.Text = Math.Max(1, (int)Math.Round(zone.LifespanSeconds / 60.0)).ToString();


    }


private static bool TryParseMinutes(
    string? text,
    out int resultSeconds)
{
    resultSeconds = 0;
    if (!int.TryParse(text, out var minutes) || minutes < 0)
        return false;

    try
    {
        resultSeconds = checked(minutes * 60);
        return true;
    }
    catch (OverflowException)
    {
        return false;
    }
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

        if (!TryParseMinutes(
                StartTimeEntry.Text,
                out var startTime))
        {
            await DisplayAlert(
                "Invalid Start Time",
                "Enter a whole number of minutes (0 or greater).",
                "OK");

            return;
        }

        _zone.RadiusMetres =
            radius;

        _zone.StartTime =
            startTime;

        if (LifespanEntry.IsEnabled)
        {
            if (!TryParseMinutes(
                    LifespanEntry.Text,
                    out var lifespanTime) ||
                lifespanTime <= 0)
            {
                await DisplayAlert(
                    "Invalid Lifespan Time",
                    "Lifespan must be greater than zero minutes.",
                    "OK");

                return;
            }

            _zone.LifespanSeconds =
                lifespanTime;
        }

        await Navigation.PopModalAsync();

        _completionSource.TrySetResult(true);
    }

    private async void Cancel_Clicked(
        object? sender,
        EventArgs e)
    {
        await Navigation.PopModalAsync();

        _completionSource.TrySetResult(false);
    }

    public Task<bool> Completion =>
    _completionSource.Task;
}