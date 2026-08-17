namespace CombatLR.mobile.Services;

public class LocalCoordinateService
{
    private const double EarthRadiusMetres = 6371008.8;

    public (double East, double North) GpsToLocal(
        double latitude,
        double longitude,
        double homeLatitude,
        double homeLongitude)
    {
        var meanLatitude =
            (latitude + homeLatitude) / 2.0 *
            Math.PI / 180.0;

        var dLatitude =
            (latitude - homeLatitude) *
            Math.PI / 180.0;

        var dLongitude =
            (longitude - homeLongitude) *
            Math.PI / 180.0;

        var north =
            dLatitude * EarthRadiusMetres;

        var east =
            dLongitude *
            Math.Cos(meanLatitude) *
            EarthRadiusMetres;

        return (east, north);
    }

    public (double Latitude, double Longitude) LocalToGps(
        double east,
        double north,
        double homeLatitude,
        double homeLongitude)
    {
        var homeLatitudeRadians =
            homeLatitude * Math.PI / 180.0;

        var latitude =
            homeLatitude +
            north / EarthRadiusMetres *
            180.0 / Math.PI;

        var longitude =
            homeLongitude +
            east /
            (EarthRadiusMetres *
             Math.Cos(homeLatitudeRadians)) *
            180.0 / Math.PI;

        return (latitude, longitude);
    }
}