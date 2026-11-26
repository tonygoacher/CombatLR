namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public event Action<Location>? PositionChanged;

    private CancellationTokenSource? _trackingCancellation;
    private Task? _trackingTask;

    public async Task<bool> StartListeningAsync()
    {
        try
        {
            var permission = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();
            if (permission != PermissionStatus.Granted)
                permission = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();

            if (permission != PermissionStatus.Granted)
                return false;

            if (_trackingCancellation is not null)
                return true;

            _trackingCancellation = new CancellationTokenSource();
            _trackingTask = TrackLocationAsync(_trackingCancellation.Token);

            return true;
        }
        catch
        {
            _trackingCancellation?.Cancel();
            _trackingCancellation?.Dispose();
            _trackingCancellation = null;
            _trackingTask = null;
            return false;
        }
    }

    public async Task StopListeningAsync()
    {
        var cancellation = _trackingCancellation;
        var trackingTask = _trackingTask;

        _trackingCancellation = null;
        _trackingTask = null;

        if (cancellation is null)
            return;

        cancellation.Cancel();

        try
        {
            if (trackingTask is not null)
                await trackingTask;
        }
        catch (OperationCanceledException)
        {
            // Expected when the editor closes.
        }
        finally
        {
            cancellation.Dispose();
        }
    }

    private async Task TrackLocationAsync(CancellationToken cancellationToken)
    {
        // Request a fresh high-accuracy fix every few seconds while the editor
        // is visible. This avoids relying solely on Android's foreground
        // LocationChanged event, which can stop delivering updates silently.
        var request = new GeolocationRequest(
            GeolocationAccuracy.High,
            TimeSpan.FromSeconds(5));

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var location = await Geolocation.Default.GetLocationAsync(
                    request,
                    cancellationToken);

                if (location is not null)
                    PositionChanged?.Invoke(location);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                break;
            }
            catch
            {
                // A temporary GPS failure should not stop subsequent polls.
            }

            try
            {
                await Task.Delay(TimeSpan.FromSeconds(2), cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                break;
            }
        }
    }
}
