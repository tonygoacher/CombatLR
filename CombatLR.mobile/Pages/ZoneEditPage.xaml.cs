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
        // A non-positive stored lifespan is displayed with the safe 5-minute default.
        LifespanEntry.Text = (zone.LifespanSeconds <= 0
            ? 5
            : Math.Max(1, (int)Math.Round(zone.LifespanSeconds / 60.0))).ToString();

        UpdateActiveValue(RadiusEntry, "Radius", "m");
    }

    private void NumericEntry_Focused(object? sender, FocusEventArgs e)
    {
        if (sender is Entry entry)
            UpdateActiveValue(entry, GetEntryCaption(entry), GetEntryUnit(entry));
    }

    private void NumericEntry_TextChanged(object? sender, TextChangedEventArgs e)
    {
        if (sender is Entry entry && entry.IsFocused)
            UpdateActiveValue(entry, GetEntryCaption(entry), GetEntryUnit(entry));
    }

    private static string GetEntryCaption(Entry entry) => entry.AutomationId switch
    {
        "RadiusEntry" => "Radius",
        "StartTimeEntry" => "Start time",
        "LifespanEntry" => "Lifespan",
        _ => "Value"
    };

    private static string GetEntryUnit(Entry entry) => entry.AutomationId switch
    {
        "RadiusEntry" => "m",
        "StartTimeEntry" or "LifespanEntry" => "min",
        _ => string.Empty
    };

    private void UpdateActiveValue(Entry entry, string caption, string unit)
    {
        ActiveValueCaption.Text = caption;
        var value = string.IsNullOrWhiteSpace(entry.Text) ? "—" : entry.Text.Trim();
        ActiveValueLabel.Text = string.IsNullOrEmpty(unit) ? value : $"{value} {unit}";
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
                    out var lifespanTime))
            {
                await DisplayAlert(
                    "Invalid Lifespan Time",
                    "Enter a whole number of minutes (0 or greater).",
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