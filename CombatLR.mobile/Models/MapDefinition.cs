using System.Text.Json.Serialization;

namespace CombatLR.mobile.Models;

public sealed class MapDefinition
{
    public int Version { get; set; }
    public MapImageDefinition Image { get; set; } = new();
    public double MetresPerPixel { get; set; }
    public MapSourceCrop SourceCrop { get; set; } = new();
    public MapPlayfield Playfield { get; set; } = new();
    public MapReference Reference { get; set; } = new();

    [JsonIgnore]
    public double RotationDegrees => 0.0;
}

public sealed class MapImageDefinition { public int Width { get; set; } public int Height { get; set; } }

public sealed class MapSourceCrop
{
    public double X { get; set; }
    public double Y { get; set; }
    public double Width { get; set; }
    public double Height { get; set; }
}

public sealed class MapPlayfield
{
    public MapGpsPoint Centre { get; set; } = new();
    public MapGpsPoint TopLeft { get; set; } = new();
    public MapGpsPoint TopRight { get; set; } = new();
    public MapGpsPoint BottomLeft { get; set; } = new();
    public MapGpsPoint BottomRight { get; set; } = new();
}

public sealed class MapReference { public List<MapReferencePoint> Points { get; set; } = new(); }

public sealed class MapReferencePoint
{
    public string Name { get; set; } = "";
    public double X { get; set; }
    public double Y { get; set; }
    public MapGpsPoint Gps { get; set; } = new();
}

public sealed class MapGpsPoint
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}
