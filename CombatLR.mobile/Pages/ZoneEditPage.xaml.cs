using CombatLR.mobile.Models;

namespace CombatLR.mobile.Pages;

public partial class ZoneEditPage : ContentPage
{
    private readonly GameZone _zone;

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

        StartTimeEntry.Text =
            FormatTime(zone.StartTime);

        TransitionTimeEntry.Text =
            FormatTime(zone.TransitionTime);

        // Zone 1 has no previous zone,
        // so transition time has no meaning.
        if (zoneNumber == 1)
        {
            TransitionTimeEntry.IsEnabled = false;
        }
    }

    private static string FormatTime(
        TimeSpan time)
    {
        return
            $"{(int)time.TotalMinutes:00}:{time.Seconds:00}";
    }

    private static bool TryParseTime(
        string? text,
        out TimeSpan result)
    {
        result = TimeSpan.Zero;

        if (string.IsNullOrWhiteSpace(text))
            return false;

        var parts =
            text.Split(':');

        if (parts.Length != 2)
            return false;

        if (!int.TryParse(
                parts[0],
                out var minutes))
        {
            return false;
        }

        if (!int.TryParse(
                parts[1],
                out var seconds))
        {
            return false;
        }

        if (minutes < 0 ||
            seconds < 0 ||
            seconds >= 60)
        {
            return false;
        }

        result =
            new TimeSpan(
                0,
                minutes,
                seconds);

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

        if (!TryParseTime(
                StartTimeEntry.Text,
                out var startTime))
        {
            await DisplayAlert(
                "Invalid Start Time",
                "Enter time as mm:ss.",
                "OK");

            return;
        }

        _zone.RadiusMetres =
            radius;

        _zone.StartTime =
            startTime;

        if (TransitionTimeEntry.IsEnabled)
        {
            if (!TryParseTime(
                    TransitionTimeEntry.Text,
                    out var transitionTime))
            {
                await DisplayAlert(
                    "Invalid Transition Time",
                    "Enter time as mm:ss.",
                    "OK");

                return;
            }

            _zone.TransitionTime =
                transitionTime;
        }

        await Navigation.PopModalAsync();
    }

    private async void Cancel_Clicked(
        object? sender,
        EventArgs e)
    {
        await Navigation.PopModalAsync();
    }
}