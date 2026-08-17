using CombatLR.mobile.Models;

namespace CombatLR.mobile.Services;

public sealed class GameObjectCoordinateService
{
    private const double EarthRadiusMetres = 6371008.8;

    public MapCoordinateService.GeoPoint
        LocalToGps(
            HomePosition home,
            double eastMetres,
            double northMetres)
    {
        var latitudeRadians =
            home.Latitude * Math.PI / 180.0;

        var latitude =
            home.Latitude +
            (northMetres / EarthRadiusMetres)
            * 180.0 / Math.PI;

        var longitude =
            home.Longitude +
            (eastMetres /
             (EarthRadiusMetres *
              Math.Cos(latitudeRadians)))
            * 180.0 / Math.PI;

        return new MapCoordinateService.GeoPoint(
            latitude,
            longitude);
    }

    public (double EastMetres, double NorthMetres)
        GpsToLocal(
            HomePosition home,
            double latitude,
            double longitude)
    {
        var meanLatitudeRadians =
            ((home.Latitude + latitude) / 2.0)
            * Math.PI / 180.0;

        var dLatitude =
            (latitude - home.Latitude)
            * Math.PI / 180.0;

        var dLongitude =
            (longitude - home.Longitude)
            * Math.PI / 180.0;

        var northMetres =
            dLatitude * EarthRadiusMetres;

        var eastMetres =
            dLongitude *
            Math.Cos(meanLatitudeRadians) *
            EarthRadiusMetres;

        return (
            eastMetres,
            northMetres);
    }
}