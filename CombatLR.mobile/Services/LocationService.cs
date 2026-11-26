namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public event Action<Location>? PositionChanged;
    public event Action<string>? TrackingFailed;

    private bool _isListening;

    public bool IsListening => _isListening;
    public int LocationEventCount { get; private set; }
    public DateTime? LastLocationEventAt { get; private set; }
    public string? LastFailure { get; private set; }

    public async Task<bool> StartListeningAsync()
    {
        if (_isListening)
            return true;

        LastFailure = null;

        try
        {
            var permission = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();
            if (permission != PermissionStatus.Granted)
                permission = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();

            if (permission != PermissionStatus.Granted)
            {
                LastFailure = "Location permission was not granted.";
                TrackingFailed?.Invoke(LastFailure!);
                return false;
            }

            Geolocation.Default.LocationChanged += OnLocationChanged;
            Geolocation.Default.ListeningFailed += OnListeningFailed;

            var request = new GeolocationListeningRequest(GeolocationAccuracy.High);
            var started = await Geolocation.Default.StartListeningForegroundAsync(request);

            _isListening = started;

            if (!started)
            {
                Geolocation.Default.LocationChanged -= OnLocationChanged;
                Geolocation.Default.ListeningFailed -= OnListeningFailed;
                LastFailure = "Android could not start GPS location updates.";
                TrackingFailed?.Invoke(LastFailure!);
            }

            return started;
        }
        catch (Exception ex)
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            Geolocation.Default.ListeningFailed -= OnListeningFailed;
            _isListening = false;
            LastFailure = $"Unable to start GPS tracking: {ex.Message}";
            TrackingFailed?.Invoke(LastFailure!);
            return false;
        }
    }

    public Task StopListeningAsync()
    {
        Geolocation.Default.LocationChanged -= OnLocationChanged;
        Geolocation.Default.ListeningFailed -= OnListeningFailed;

        try
        {
            Geolocation.Default.StopListeningForeground();
        }
        catch (Exception ex)
        {
            LastFailure = $"Unable to stop GPS tracking: {ex.Message}";
            TrackingFailed?.Invoke(LastFailure);
        }
        finally
        {
            _isListening = false;
        }

        return Task.CompletedTask;
    }

    private void OnLocationChanged(object? sender, GeolocationLocationChangedEventArgs e)
    {
        LocationEventCount++;
        LastLocationEventAt = DateTime.Now;
        PositionChanged?.Invoke(e.Location);
    }

    private void OnListeningFailed(object? sender, GeolocationListeningFailedEventArgs e)
    {
        _isListening = false;
        LastFailure = $"GPS updates stopped: {e.Error}";
        TrackingFailed?.Invoke(LastFailure!);
    }
}
