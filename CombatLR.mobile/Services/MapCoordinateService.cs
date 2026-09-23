using CombatLR.mobile.Models;

namespace CombatLR.mobile.Services;

public sealed class MapCoordinateService
{
    private const double EarthRadiusMetres = 6371008.8;

    private readonly MapDefinition _map;

    // MapBuilder's affine transform, in source-image pixels:
    // east  = E0 + Ex*x + Ey*y
    // north = N0 + Nx*x + Ny*y
    private double _eastOffset;
    private double _eastPerPixelX;
    private double _eastPerPixelY;
    private double _northOffset;
    private double _northPerPixelX;
    private double _northPerPixelY;
    private bool _transformValid;

    public MapCoordinateService(MapDefinition map)
    {
        _map = map ?? throw new ArgumentNullException(nameof(map));
        BuildTransform();
    }

    private void BuildTransform()
    {
        _transformValid = false;

        var points = _map.Reference?.Points;
        if (points is null || points.Count < 3)
            return;

        // MapBuilder defines local EN coordinates relative to point A.
        var gpsA = points[0].Gps;
        var latA = gpsA.Latitude * Math.PI / 180.0;
        var lonA = gpsA.Longitude * Math.PI / 180.0;

        // Normal equations for z = a*x + b*y + c.
        double sxx = 0, sxy = 0, sx = 0;
        double syy = 0, sy = 0, sn = points.Count;
        double sxe = 0, sye = 0, se = 0;
        double sxn = 0, syn = 0, snorth = 0;

        foreach (var point in points)
        {
            var x = point.X;
            var y = point.Y;
            var lat = point.Gps.Latitude * Math.PI / 180.0;
            var lon = point.Gps.Longitude * Math.PI / 180.0;

            var east =
                (lon - lonA) * Math.Cos(latA) * EarthRadiusMetres;

            var north =
                (lat - latA) * EarthRadiusMetres;

            sxx += x * x;
            sxy += x * y;
            sx += x;
            syy += y * y;
            sy += y;

            sxe += x * east;
            sye += y * east;
            se += east;

            sxn += x * north;
            syn += y * north;
            snorth += north;
        }

        var matrix = new double[,]
        {
            { sxx, sxy, sx },
            { sxy, syy, sy },
            { sx,  sy,  sn }
        };

        var eastResult = Solve3x3(matrix, new[] { sxe, sye, se });
        var northResult = Solve3x3(matrix, new[] { sxn, syn, snorth });

        if (eastResult is null || northResult is null)
            return;

        _eastPerPixelX = eastResult[0];
        _eastPerPixelY = eastResult[1];
        _eastOffset = eastResult[2];

        _northPerPixelX = northResult[0];
        _northPerPixelY = northResult[1];
        _northOffset = northResult[2];

        _transformValid = true;
    }

    private static double[]? Solve3x3(double[,] input, double[] rhs)
    {
        var a = new double[3, 4];

        for (var row = 0; row < 3; row++)
        {
            for (var col = 0; col < 3; col++)
                a[row, col] = input[row, col];

            a[row, 3] = rhs[row];
        }

        // Gaussian elimination with partial pivoting.
        for (var col = 0; col < 3; col++)
        {
            var pivot = col;
            var max = Math.Abs(a[col, col]);

            for (var row = col + 1; row < 3; row++)
            {
                var value = Math.Abs(a[row, col]);
                if (value > max)
                {
                    max = value;
                    pivot = row;
                }
            }

            if (max < 1e-12)
                return null;

            if (pivot != col)
            {
                for (var k = col; k < 4; k++)
                    (a[col, k], a[pivot, k]) = (a[pivot, k], a[col, k]);
            }

            var divisor = a[col, col];
            for (var k = col; k < 4; k++)
                a[col, k] /= divisor;

            for (var row = 0; row < 3; row++)
            {
                if (row == col)
                    continue;

                var factor = a[row, col];
                for (var k = col; k < 4; k++)
                    a[row, k] -= factor * a[col, k];
            }
        }

        return new[] { a[0, 3], a[1, 3], a[2, 3] };
    }

    public GeoPoint ImageToGps(double exportedX, double exportedY)
    {
        EnsureValid();

        var source = ExportedToSource(exportedX, exportedY);

        var east =
            _eastOffset +
            _eastPerPixelX * source.X +
            _eastPerPixelY * source.Y;

        var north =
            _northOffset +
            _northPerPixelX * source.X +
            _northPerPixelY * source.Y;

        var gpsA = _map.Reference.Points[0].Gps;
        var latA = gpsA.Latitude * Math.PI / 180.0;

        var latitude =
            gpsA.Latitude +
            north / EarthRadiusMetres * 180.0 / Math.PI;

        var longitude =
            gpsA.Longitude +
            east / (EarthRadiusMetres * Math.Cos(latA)) * 180.0 / Math.PI;

        return new GeoPoint(latitude, longitude);
    }

    public Point GpsToImage(double latitude, double longitude)
    {
        EnsureValid();

        var gpsA = _map.Reference.Points[0].Gps;
        var latA = gpsA.Latitude * Math.PI / 180.0;

        var east =
            (longitude - gpsA.Longitude) *
            Math.PI / 180.0 *
            Math.Cos(latA) *
            EarthRadiusMetres;

        var north =
            (latitude - gpsA.Latitude) *
            Math.PI / 180.0 *
            EarthRadiusMetres;

        var determinant =
            _eastPerPixelX * _northPerPixelY -
            _eastPerPixelY * _northPerPixelX;

        if (Math.Abs(determinant) < 1e-12)
            throw new InvalidOperationException(
                "Map coordinate transform cannot be inverted.");

        var adjustedEast = east - _eastOffset;
        var adjustedNorth = north - _northOffset;

        var sourceX =
            (adjustedEast * _northPerPixelY -
             _eastPerPixelY * adjustedNorth) /
            determinant;

        var sourceY =
            (_eastPerPixelX * adjustedNorth -
             adjustedEast * _northPerPixelX) /
            determinant;

        return SourceToExported(sourceX, sourceY);
    }

    private Point ExportedToSource(double exportedX, double exportedY)
    {
        var crop = _map.SourceCrop;

        if (crop.Width <= 0 || crop.Height <= 0 ||
            _map.Image.Width <= 0 || _map.Image.Height <= 0)
        {
            throw new InvalidOperationException(
                "Map image/crop dimensions are invalid.");
        }

        return new Point(
            crop.X + exportedX * crop.Width / _map.Image.Width,
            crop.Y + exportedY * crop.Height / _map.Image.Height);
    }

    private Point SourceToExported(double sourceX, double sourceY)
    {
        var crop = _map.SourceCrop;

        if (crop.Width <= 0 || crop.Height <= 0 ||
            _map.Image.Width <= 0 || _map.Image.Height <= 0)
        {
            throw new InvalidOperationException(
                "Map image/crop dimensions are invalid.");
        }

        return new Point(
            (sourceX - crop.X) * _map.Image.Width / crop.Width,
            (sourceY - crop.Y) * _map.Image.Height / crop.Height);
    }

    private void EnsureValid()
    {
        if (!_transformValid)
            throw new InvalidOperationException(
                "Map coordinate transform is not valid.");
    }

    public readonly record struct GeoPoint(double Latitude, double Longitude);
}
