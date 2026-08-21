namespace CombatLR.mobile.Models;

public class GamePlan
{
    public GamePlan()
    {
        _nextPickupId = 0;
    }
    public int Version { get; set; } = 1;

    public HomePosition? Home { get; set; }

    public List<GameZone> Zones { get; set; } = new();

    private int _nextPickupId = 0;

    public int AllocatePickupId()
    {
        return _nextPickupId++;
    }
}

public class HomePosition
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}