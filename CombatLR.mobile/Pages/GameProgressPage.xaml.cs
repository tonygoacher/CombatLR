using System.Globalization;
using CombatLR.mobile.Models;
using CombatLR.mobile.Services;

namespace CombatLR.mobile.Pages;

public partial class GameProgressPage : ContentPage
{
    private readonly SelectedGamePlanService _selectedPlan;
    private readonly GameSession _session;
    private readonly IDispatcherTimer _timer;
    private int _locationTick;

    public GameProgressPage(SelectedGamePlanService selectedPlan)
    {
        InitializeComponent();

        _selectedPlan = selectedPlan;

        var plan = _selectedPlan.GamePlan
            ?? throw new InvalidOperationException("No game plan is selected.");

        _session = new GameSessionService().GetSession(
            _selectedPlan.ProjectName,
            plan.Version)
            ?? throw new InvalidOperationException("This game plan has not been started.");

        PlanLabel.Text =
            $"PLAN: {_selectedPlan.ProjectName}  v{plan.Version}";

        GameStartLabel.Text =
            $"LAST GAME START: {_session.StartedAtUtc.ToLocalTime():HH:mm:ss} " +
            $"({_session.StartedAtUtc:HH:mm:ss} UTC)";

        MapNameLabel.Text = _selectedPlan.ProjectName ?? "";
        MapImage.Source = _selectedPlan.MapBitmapPath;

        MapContainer.SizeChanged += MapContainer_SizeChanged;

        _timer = Dispatcher.CreateTimer();
        _timer.Interval = TimeSpan.FromSeconds(1);
        _timer.Tick += (_, _) =>
        {
            UpdateProgress();
            _locationTick++;

            if (_locationTick >= 10)
            {
                _locationTick = 0;
                _ = UpdateLocationAsync();
            }
        };
    }

    protected override async void OnAppearing()
    {
        base.OnAppearing();
        UpdateProgress();
        _timer.Start();
        await UpdateLocationAsync();
    }

    protected override void OnDisappearing()
    {
        _timer.Stop();
        base.OnDisappearing();
    }

    private void UpdateProgress()
    {
        var plan = _selectedPlan.GamePlan;
        if (plan is null)
            return;

        int elapsedSinceStart = Math.Max(
            0,
            (int)Math.Floor(
                (DateTimeOffset.UtcNow - _session.StartedAtUtc).TotalSeconds));

        int delay = Math.Max(0, plan.GameDelayTime);

        if (elapsedSinceStart < delay)
        {
            int remaining = delay - elapsedSinceStart;
            GamePhaseLabel.Text = "DISPERSAL";
            ProgressTimerLabel.Text = FormatDuration(remaining);
            GameElapsedLabel.Text =
                $"GAME BEGINS IN {FormatDuration(remaining)}";
            CurrentZoneLabel.Text = "Not started";
            TimeLeftLabel.Text = "—";
            // Zone times begin only after the dispersal delay, so include
            // the remaining delay in the first-zone countdown.
            NextZoneLabel.Text = GetFirstZoneCountdown(plan.Zones, -remaining);
            TransitionLabel.Text = "";
            return;
        }

        int gameTime = elapsedSinceStart - delay;

        GamePhaseLabel.Text = "GAME IN PROGRESS";
        ProgressTimerLabel.Text = FormatDuration(gameTime);
        GameElapsedLabel.Text = "ELAPSED GAME TIME";

        UpdateZoneStatus(plan.Zones, gameTime);
    }

    private void UpdateZoneStatus(
        IReadOnlyCollection<GameZone> sourceZones,
        int gameTime)
    {
        var zones = sourceZones
            .OrderBy(zone => zone.StartTime)
            .ToList();

        if (zones.Count == 0)
        {
            CurrentZoneLabel.Text = "No zones";
            TimeLeftLabel.Text = "—";
            NextZoneLabel.Text = "—";
            TransitionLabel.Text = "";
            return;
        }

        int currentIndex = zones.FindLastIndex(
            zone => zone.StartTime <= gameTime);

        if (currentIndex < 0)
        {
            CurrentZoneLabel.Text = "Not started";
            TimeLeftLabel.Text = "—";
            NextZoneLabel.Text = FormatDuration(
                Math.Max(0, zones[0].StartTime - gameTime));
            TransitionLabel.Text = "";
            return;
        }

        var current = zones[currentIndex];
        GameZone? next = currentIndex + 1 < zones.Count
            ? zones[currentIndex + 1]
            : null;

        int currentEnd = current.LifespanSeconds == 0
            ? int.MaxValue
            : current.StartTime + current.LifespanSeconds;

        // When zones overlap, the old zone remains current only until
        // the next zone's start time (the transition completes there).
        int activeEnd = next is null
            ? currentEnd
            : Math.Min(currentEnd, next.StartTime);

        // A zone whose lifespan has expired is no longer active while the
        // next zone is still waiting to start.
        if (gameTime >= activeEnd)
        {
            CurrentZoneLabel.Text = "No active zone";
            TimeLeftLabel.Text = "Expired";
            NextZoneLabel.Text = next is null
                ? "No further zones"
                : FormatDuration(Math.Max(0, next.StartTime - gameTime));
            TransitionLabel.Text = "";
            return;
        }

        CurrentZoneLabel.Text = $"Zone {currentIndex + 1}";
        TimeLeftLabel.Text = activeEnd == int.MaxValue
            ? "No expiry"
            : FormatDuration(Math.Max(0, activeEnd - gameTime));

        if (next is null)
        {
            NextZoneLabel.Text = "No further zones";
            TransitionLabel.Text = "";
            return;
        }

        int timeToNext = Math.Max(0, next.StartTime - gameTime);
        NextZoneLabel.Text = FormatDuration(timeToNext);

        int overlap = currentEnd == int.MaxValue
            ? 0
            : Math.Max(0, currentEnd - next.StartTime);

        int transitionStart = next.StartTime - overlap;

        if (overlap > 0 &&
            gameTime >= transitionStart &&
            gameTime < next.StartTime)
        {
            double progress = (double)(gameTime - transitionStart) / overlap;
            TransitionLabel.Text =
                $"TRANSITION TO ZONE {currentIndex + 2}  " +
                $"{progress * 100:0}%";
        }
        else
        {
            TransitionLabel.Text = "";
        }
    }

    private static string GetFirstZoneCountdown(
        IEnumerable<GameZone> zones,
        int gameTime)
    {
        var first = zones.OrderBy(zone => zone.StartTime).FirstOrDefault();
        return first is null
            ? "No zones"
            : FormatDuration(Math.Max(0, first.StartTime - gameTime));
    }

    private static string FormatDuration(int totalSeconds)
    {
        totalSeconds = Math.Max(0, totalSeconds);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        return hours > 0
            ? $"{hours:00}:{minutes:00}:{seconds:00}"
            : $"{minutes:00}:{seconds:00}";
    }

    private void MapContainer_SizeChanged(object? sender, EventArgs e)
    {
        var image = _selectedPlan.MapDefinition?.Image;
        if (image is null || image.Width <= 0 || image.Height <= 0 ||
            MapContainer.Width <= 0)
        {
            return;
        }

        MapContainer.HeightRequest =
            MapContainer.Width * image.Height / image.Width;
    }

    private async Task UpdateLocationAsync()
    {
        try
        {
            var status =
                await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();

            if (status != PermissionStatus.Granted)
            {
                status =
                    await Permissions.RequestAsync<Permissions.LocationWhenInUse>();
            }

            if (status != PermissionStatus.Granted)
            {
                LocationLabel.Text = "Location permission not granted";
                PositionMarker.IsVisible = false;
                return;
            }

            var location = await Geolocation.Default.GetLocationAsync(
                new GeolocationRequest(
                    GeolocationAccuracy.Medium,
                    TimeSpan.FromSeconds(5)));

            if (location is null)
            {
                LocationLabel.Text = "GPS position unavailable";
                PositionMarker.IsVisible = false;
                return;
            }

            LocationLabel.Text =
                $"GPS: {location.Latitude.ToString("F6", CultureInfo.InvariantCulture)}, " +
                $"{location.Longitude.ToString("F6", CultureInfo.InvariantCulture)}";

            UpdateMapPosition(location.Latitude, location.Longitude);
        }
        catch (Exception ex)
        {
            LocationLabel.Text = $"GPS unavailable: {ex.Message}";
            PositionMarker.IsVisible = false;
        }
    }

    private void UpdateMapPosition(double latitude, double longitude)
    {
        var playfield = _selectedPlan.MapDefinition?.Playfield;
        if (playfield is null)
        {
            PositionMarker.IsVisible = false;
            return;
        }

        var topLeft = playfield.TopLeft;
        var topRight = playfield.TopRight;
        var bottomLeft = playfield.BottomLeft;

        static (double East, double North) ToLocal(
            MapGpsPoint origin,
            double lat,
            double lon)
        {
            double averageLatitude =
                (origin.Latitude + lat) * 0.5 * Math.PI / 180.0;

            double east =
                (lon - origin.Longitude) *
                111320.0 * Math.Cos(averageLatitude);

            double north =
                (lat - origin.Latitude) * 111132.0;

            return (east, north);
        }

        var top = ToLocal(topLeft, topRight.Latitude, topRight.Longitude);
        var left = ToLocal(topLeft, bottomLeft.Latitude, bottomLeft.Longitude);
        var point = ToLocal(topLeft, latitude, longitude);

        // Solve point = u * (top-right - top-left) +
        //               v * (bottom-left - top-left).
        double determinant =
            top.East * left.North - top.North * left.East;

        if (Math.Abs(determinant) < 0.000001)
        {
            PositionMarker.IsVisible = false;
            return;
        }

        double u =
            (point.East * left.North - point.North * left.East) /
            determinant;

        double v =
            (top.East * point.North - top.North * point.East) /
            determinant;

        PositionMarker.IsVisible =
            u >= 0 && u <= 1 && v >= 0 && v <= 1;

        if (!PositionMarker.IsVisible)
            return;

        AbsoluteLayout.SetLayoutFlags(
            PositionMarker,
            AbsoluteLayoutFlags.PositionProportional);

        AbsoluteLayout.SetLayoutBounds(
            PositionMarker,
            new Rect(u, v, 18, 18));

        PositionMarker.TranslationX = -9;
        PositionMarker.TranslationY = -9;
    }
}
