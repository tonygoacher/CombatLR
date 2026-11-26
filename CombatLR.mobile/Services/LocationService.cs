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
            var started = await Geolocation.Default.StartListeningForegroundAsync(
                GeolocationAccuracy.High,
                TimeSpan.FromSeconds(2));

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

    public async Task StopListeningAsync()
    {
        if (!_isListening)
            return;

        try
        {
            await Geolocation.Default.StopListeningForegroundAsync();
        }
        catch
        {
            // Best-effort shutdown during page navigation.
        }
        finally
        {
            Geolocation.Default.LocationChanged -= OnLocationChanged;
            _isListening = false;
        }
    }

    private void OnLocationChanged(object? sender, GeolocationLocationChangedEventArgs e)
    {
        PositionChanged?.Invoke(e.Location);
    }
}
