namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public event Action<Location>? PositionChanged;
    public event Action<string>? TrackingFailed;

    private bool _isListening;

    public async Task<bool> StartListeningAsync()
    {
        if (_isListening)
            return true;

        try
        {
            var permission = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();
            if (permission != PermissionStatus.Granted)
                permission = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();

            if (permission != PermissionStatus.Granted)
            {
                TrackingFailed?.Invoke("Location permission was not granted.");
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
                TrackingFailed?.Invoke("Android could not start GPS location updates.");
            }

            return started;
        }
        catch (Exception ex)
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            Geolocation.Default.ListeningFailed -= OnListeningFailed;
            _isListening = false;
            TrackingFailed?.Invoke($"Unable to start GPS tracking: {ex.Message}");
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
            TrackingFailed?.Invoke($"Unable to stop GPS tracking: {ex.Message}");
        }
        finally
        {
            _isListening = false;
        }

        return Task.CompletedTask;
    }

    private void OnLocationChanged(object? sender, GeolocationLocationChangedEventArgs e)
    {
        PositionChanged?.Invoke(e.Location);
    }

    private void OnListeningFailed(object? sender, GeolocationListeningFailedEventArgs e)
    {
        _isListening = false;
        TrackingFailed?.Invoke($"GPS updates stopped: {e.Error}");
    }
}
