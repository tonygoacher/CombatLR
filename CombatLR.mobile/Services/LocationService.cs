namespace CombatLR.mobile.Services;

public sealed class LocationService
{
    public enum LocationStatus
    {
        Available,
        PermissionDenied,
        Unavailable,
        Error
    }

    public readonly record struct LocationResult(
        LocationStatus Status,
        MapCoordinateService.GeoPoint? Position);

    public async Task<LocationResult>
        GetCurrentPositionAsync()
    {
        try
        {
            var permission =
                await Permissions.CheckStatusAsync<
                    Permissions.LocationWhenInUse>();

            if (permission != PermissionStatus.Granted)
            {
                permission =
                    await Permissions.RequestAsync<
                        Permissions.LocationWhenInUse>();
            }

            if (permission != PermissionStatus.Granted)
            {
                return new LocationResult(
                    LocationStatus.PermissionDenied,
                    null);
            }

            var location =
                await Geolocation.Default.GetLocationAsync(
                    new GeolocationRequest(
                        GeolocationAccuracy.High,
                        TimeSpan.FromSeconds(5)));

            if (location is null)
            {
                return new LocationResult(
                    LocationStatus.Unavailable,
                    null);
            }

            return new LocationResult(
                LocationStatus.Available,
                new MapCoordinateService.GeoPoint(
                    location.Latitude,
                    location.Longitude));
        }
        catch (Exception e)
        {
            return new LocationResult(
                LocationStatus.Error,
                null);
        }
    }
}