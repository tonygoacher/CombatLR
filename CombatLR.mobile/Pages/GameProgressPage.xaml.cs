using Microsoft.Maui.Layouts;
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
    private bool _locationPermissionRequested;

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

            // The first zone is already defined in the plan and should be
            // visible on the map while players are dispersing, even though
            // the zone clock itself has not started yet.
            CurrentZoneLabel.Text = plan.Zones.Count > 0
                ? "Zone 1 (upcoming)"
                : "No zones";
            TimeLeftLabel.Text = "—";
            UpdateZoneMap(plan.Zones, 0, showFirstZone: true);
            return;
        }

        int gameTime = elapsedSinceStart - delay;

        GamePhaseLabel.Text = "GAME IN PROGRESS";
        ProgressTimerLabel.Text = FormatDuration(gameTime);
        GameElapsedLabel.Text = "ELAPSED GAME TIME";

        UpdateZoneStatus(plan.Zones, gameTime);
        UpdateZoneMap(plan.Zones, gameTime);
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

    private void UpdateZoneMap(
        IReadOnlyCollection<GameZone> sourceZones,
        int gameTime,
        bool showFirstZone = false)
    {
        ZoneCircle.IsVisible = false;

        var plan = _selectedPlan.GamePlan;
        var home = plan?.Home;
        if (home is null || MapContainer.Width <= 0 || MapContainer.Height <= 0)
            return;

        // Keep the same ordering and hand-over rules as UpdateZoneStatus
        // and the firmware ZoneEngine.
        var zones = sourceZones.OrderBy(zone => zone.StartTime).ToList();
        int currentIndex = showFirstZone
            ? (zones.Count > 0 ? 0 : -1)
            : zones.FindLastIndex(zone => zone.StartTime <= gameTime);
        if (currentIndex < 0)
            return;

        var from = zones[currentIndex];
        GameZone? to = currentIndex + 1 < zones.Count ? zones[currentIndex + 1] : null;

        double east = from.EastMetres;
        double north = from.NorthMetres;
        double radius = from.RadiusMetres;

        int fromEnd = from.LifespanSeconds == 0
            ? int.MaxValue
            : from.StartTime + from.LifespanSeconds;

        int activeEnd = to is null ? fromEnd : Math.Min(fromEnd, to.StartTime);
        if (!showFirstZone && gameTime >= activeEnd)
            return;

        // During dispersal, show the first zone at its planned geometry.
        // Once the game clock starts, interpolate the circle exactly as the
        // firmware does: it starts at next.StartTime - overlap and finishes
        // at next.StartTime.
        if (!showFirstZone &&
            to is not null && fromEnd != int.MaxValue && to.StartTime < fromEnd)
        {
            int overlap = fromEnd - to.StartTime;
            int transitionStart = to.StartTime - overlap;

            if (gameTime >= transitionStart && gameTime < to.StartTime)
            {
                double progress = (double)(gameTime - transitionStart) / overlap;
                east += (to.EastMetres - east) * progress;
                north += (to.NorthMetres - north) * progress;
                radius += (to.RadiusMetres - radius) * progress;
            }
        }

        if (!TryProjectGamePosition(east, north, home, out double u, out double v))
            return;

        // Project four cardinal points around the world-space circle. This
        // gives its screen-space extents even when the map is slightly skewed.
        if (!TryProjectGamePosition(east + radius, north, home, out double eastU, out double eastV) ||
            !TryProjectGamePosition(east - radius, north, home, out double westU, out double westV) ||
            !TryProjectGamePosition(east, north + radius, home, out double northU, out double northV) ||
            !TryProjectGamePosition(east, north - radius, home, out double southU, out double southV))
            return;

        double radiusX = Math.Max(
            Math.Max(Math.Abs(eastU - u), Math.Abs(westU - u)),
            Math.Max(Math.Abs(northU - u), Math.Abs(southU - u))) * MapContainer.Width;

        double radiusY = Math.Max(
            Math.Max(Math.Abs(eastV - v), Math.Abs(westV - v)),
            Math.Max(Math.Abs(northV - v), Math.Abs(southV - v))) * MapContainer.Height;

        if (!double.IsFinite(radiusX) || !double.IsFinite(radiusY) ||
            radiusX <= 0 || radiusY <= 0)
            return;

        AbsoluteLayout.SetLayoutFlags(ZoneCircle, AbsoluteLayoutFlags.None);
        AbsoluteLayout.SetLayoutBounds(
            ZoneCircle,
            new Rect(
                u * MapContainer.Width - radiusX,
                v * MapContainer.Height - radiusY,
                radiusX * 2,
                radiusY * 2));

        ZoneCircle.IsVisible = true;
    }

    private bool TryProjectGamePosition(
        double eastMetres,
        double northMetres,
        HomePosition home,
        out double u,
        out double v)
    {
        double latitude = home.Latitude + northMetres / 111132.0;
        double longitudeScale = 111320.0 * Math.Cos(home.Latitude * Math.PI / 180.0);

        if (Math.Abs(longitudeScale) < 0.001)
        {
            u = v = 0;
            return false;
        }

        double longitude = home.Longitude + eastMetres / longitudeScale;
        return TryProjectGpsToMap(latitude, longitude, out u, out v);
    }

    private bool TryProjectGpsToMap(double latitude, double longitude, out double u, out double v)
    {
        // A valid GPS fix can be anywhere in the world. Reject malformed
        // coordinates safely; distant but valid coordinates are handled by
        // the map-boundary check in UpdateMapPosition.
        if (!double.IsFinite(latitude) || !double.IsFinite(longitude) ||
            latitude < -90 || latitude > 90 ||
            longitude < -180 || longitude > 180)
        {
            u = v = 0;
            return false;
        }

        var playfield = _selectedPlan.MapDefinition?.Playfield;
        if (playfield is null)
        {
            u = v = 0;
            return false;
        }

        var topLeft = playfield.TopLeft;
        var topRight = playfield.TopRight;
        var bottomLeft = playfield.BottomLeft;

        static (double East, double North) ToLocal(MapGpsPoint origin, double lat, double lon)
        {
            double averageLatitude = (origin.Latitude + lat) * 0.5 * Math.PI / 180.0;
            double east = (lon - origin.Longitude) * 111320.0 * Math.Cos(averageLatitude);
            double north = (lat - origin.Latitude) * 111132.0;
            return (east, north);
        }

        var top = ToLocal(topLeft, topRight.Latitude, topRight.Longitude);
        var left = ToLocal(topLeft, bottomLeft.Latitude, bottomLeft.Longitude);
        var point = ToLocal(topLeft, latitude, longitude);

        double determinant = top.East * left.North - top.North * left.East;
        if (!double.IsFinite(determinant) || Math.Abs(determinant) < 0.000001)
        {
            u = v = 0;
            return false;
        }

        u = (point.East * left.North - point.North * left.East) / determinant;
        v = (top.East * point.North - top.North * point.East) / determinant;
        return double.IsFinite(u) && double.IsFinite(v);
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

            if (status != PermissionStatus.Granted &&
                !_locationPermissionRequested)
            {
                _locationPermissionRequested = true;
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
        if (!TryProjectGpsToMap(latitude, longitude, out double u, out double v) ||
            !double.IsFinite(u) || !double.IsFinite(v) ||
            u < 0 || u > 1 || v < 0 || v > 1)
        {
            PositionMarker.IsVisible = false;
            return;
        }

        PositionMarker.IsVisible = true;
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
