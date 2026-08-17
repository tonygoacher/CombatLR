using CombatLR.mobile.Models;

namespace CombatLR.mobile.Services;

public sealed class MapCoordinateService
{
    private const double EarthRadiusMetres = 6371008.8;

    private readonly MapDefinition _map;

    private double _eastPerPixelX;
    private double _eastPerPixelY;
    private double _northPerPixelX;
    private double _northPerPixelY;

    private bool _transformValid;

    public MapCoordinateService(MapDefinition map)
    {
        _map = map;

        BuildTransform();
    }

    private void BuildTransform()
    {
        var pointA = _map.Reference.PointA;
        var pointB = _map.Reference.PointB;

        var gpsA = _map.Reference.GpsA;
        var gpsB = _map.Reference.GpsB;

        if (gpsA is null || gpsB is null)
            return;

        var meanLat =
            ((gpsA.Latitude + gpsB.Latitude) / 2.0)
            * Math.PI / 180.0;

        var dLon =
            (gpsB.Longitude - gpsA.Longitude)
            * Math.PI / 180.0;

        var dLat =
            (gpsB.Latitude - gpsA.Latitude)
            * Math.PI / 180.0;

        var east =
            dLon * Math.Cos(meanLat) * EarthRadiusMetres;

        var north =
            dLat * EarthRadiusMetres;

        var dx = pointB.X - pointA.X;
        var dy = pointB.Y - pointA.Y;

        var pixelDistance =
            Math.Sqrt(dx * dx + dy * dy);

        var geoDistance =
            Math.Sqrt(east * east + north * north);

        if (pixelDistance < 0.000001 ||
            geoDistance < 0.000001)
        {
            return;
        }

        // Unit vector along A -> B in image coordinates.
        var ix = dx / pixelDistance;
        var iy = dy / pixelDistance;

        // Unit vector along A -> B in geographic
        // east/north coordinates.
        var gx = east / geoDistance;
        var gy = north / geoDistance;

        var scale =
            geoDistance / pixelDistance;

        // Same 2-D transform used by MapBuilder.
        //
        // The exported image uses screen coordinates:
        // +X = right
        // +Y = down.
        //
        // The transform therefore maps the complete
        // image plane, not just the A -> B direction.

        _eastPerPixelX =
            scale * (ix * gx - iy * gy);

        _eastPerPixelY =
            scale * (iy * gx + ix * gy);

        _northPerPixelX =
            scale * (ix * gy + iy * gx);

        _northPerPixelY =
            scale * (iy * gy - ix * gx);

        _transformValid = true;
    }

    public GeoPoint ImageToGps(
        double exportedX,
        double exportedY)
    {
        if (!_transformValid)
            throw new InvalidOperationException(
                "Map coordinate transform is not valid.");

        /*
         * The tap coordinates are in the exported 1280×960
         * bitmap.
         *
         * Reference.PointA and PointB in map.json are in
         * SOURCE IMAGE coordinates.
         *
         * Convert exported coordinates back into source
         * image coordinates first.
         */

        var sourceCrop = _map.SourceCrop;

        var exportedWidth =
            _map.Image.Width;

        var exportedHeight =
            _map.Image.Height;

        if (exportedWidth <= 0 ||
            exportedHeight <= 0)
        {
            throw new InvalidOperationException(
                "Map image dimensions are invalid.");
        }

        var sourceX =
            sourceCrop.X +
            exportedX *
            sourceCrop.Width /
            exportedWidth;

        var sourceY =
            sourceCrop.Y +
            exportedY *
            sourceCrop.Height /
            exportedHeight;

        // Now perform the calibrated source-image
        // coordinate -> GPS conversion.

        var pointA =
            _map.Reference.PointA;

        var gpsA =
            _map.Reference.GpsA;

        if (gpsA is null)
            throw new InvalidOperationException(
                "GPS reference point A is missing.");

        var dx =
            sourceX - pointA.X;

        var dy =
            sourceY - pointA.Y;

        var east =
            _eastPerPixelX * dx +
            _eastPerPixelY * dy;

        var north =
            _northPerPixelX * dx +
            _northPerPixelY * dy;

        var latitude =
            gpsA.Latitude +
            (north / EarthRadiusMetres)
            * 180.0 / Math.PI;

        var longitude =
            gpsA.Longitude +
            (east /
             (EarthRadiusMetres *
              Math.Cos(
                  gpsA.Latitude *
                  Math.PI / 180.0)))
            * 180.0 / Math.PI;

        return new GeoPoint(
            latitude,
            longitude);
    }

    public readonly record struct GeoPoint(
        double Latitude,
        double Longitude);
}