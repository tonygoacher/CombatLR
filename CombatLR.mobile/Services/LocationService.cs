namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public event Action<Location>? PositionChanged;

    private bool _isListening;

    public bool IsListening => _isListening;

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
                return false;

            Geolocation.Default.LocationChanged += OnLocationChanged;
            Geolocation.Default.ListeningFailed += OnListeningFailed;

            var request = new GeolocationListeningRequest(GeolocationAccuracy.High);
            var started = await Geolocation.Default.StartListeningForegroundAsync(request);

            _isListening = started;

            if (!started)
            {
                Geolocation.Default.LocationChanged -= OnLocationChanged;
                Geolocation.Default.ListeningFailed -= OnListeningFailed;
            }

            return started;
        }
        catch (Exception)
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            Geolocation.Default.ListeningFailed -= OnListeningFailed;
            _isListening = false;
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
    }
}
