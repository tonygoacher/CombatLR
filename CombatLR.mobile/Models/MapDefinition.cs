using System.Text.Json.Serialization;

namespace CombatLR.mobile.Models;

public sealed class MapDefinition
{
    public int Version { get; set; }

    public MapImageDefinition Image { get; set; } = new();

    public double RotationDegrees { get; set; }

    public double MetresPerPixel { get; set; }

    public MapRectangle SourceCrop { get; set; } = new();

    public PlayfieldDefinition Playfield { get; set; } = new();

    public MapReferenceDefinition Reference { get; set; } = new();
}

public sealed class MapImageDefinition
{
    public int Width { get; set; }
    public int Height { get; set; }
}

public sealed class MapRectangle
{
    public double X { get; set; }
    public double Y { get; set; }
    public double Width { get; set; }
    public double Height { get; set; }
}

public sealed class PlayfieldDefinition
{
    public GeoPoint Centre { get; set; } = new();
    public GeoPoint TopLeft { get; set; } = new();
    public GeoPoint TopRight { get; set; } = new();
    public GeoPoint BottomLeft { get; set; } = new();
    public GeoPoint BottomRight { get; set; } = new();
}

public sealed class GeoPoint
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}

public sealed class MapReferenceDefinition
{
    public PixelPoint PointA { get; set; } = new();
    public PixelPoint PointB { get; set; } = new();

    public double DistanceMetres { get; set; }

    public GeoPoint GpsA { get; set; } = new();
    public GeoPoint GpsB { get; set; } = new();

    public double BearingDegrees { get; set; }
}

public sealed class PixelPoint
{
    public double X { get; set; }
    public double Y { get; set; }
}