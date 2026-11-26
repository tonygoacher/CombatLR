namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public event Action<Location>? PositionChanged;

    private bool _isListening;

    public async Task<bool> StartListeningAsync()
    {
        try
        {
            var permission = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();
            if (permission != PermissionStatus.Granted)
                permission = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();

            if (permission != PermissionStatus.Granted)
                return false;

            if (_isListening)
                return true;

            Geolocation.Default.LocationChanged += OnLocationChanged;

            var request = new GeolocationListeningRequest(GeolocationAccuracy.High);
            var started = await Geolocation.Default.StartListeningForegroundAsync(request);

            _isListening = started;
            if (!started)
                Geolocation.Default.LocationChanged -= OnLocationChanged;

            return started;
        }
        catch
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            _isListening = false;
            return false;
        }
    }

    public Task StopListeningAsync()
    {
        if (!_isListening)
            return Task.CompletedTask;

        try
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            Geolocation.Default.StopListeningForeground();
        }
        catch
        {
            // Best-effort shutdown during page navigation.
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
}
