namespace CombatLR.mobile.Models;

public class GamePlan
{
    public int Version { get; set; } = 1;

    public HomePosition? Home { get; set; }

    public List<GameZone> Zones { get; set; } = new();
}

public class HomePosition
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}