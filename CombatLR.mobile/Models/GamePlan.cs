using System.Text.Json.Serialization;

namespace CombatLR.mobile.Models;

public class GamePlan
{
    public GamePlan()
    {
        _nextPickupId = 0;
    }

    public int Version { get; set; } = 1;

    [JsonPropertyName("gameDelayTime")]
    public int GameDelayTime { get; set; } = 0;

    public HomePosition? Home { get; set; }

    public List<GameZone> Zones { get; set; } = new();

    private int _nextPickupId = 0;

    public int AllocatePickupId()
    {
        return _nextPickupId++;
    }

    public void InitialisePickupIdAllocator()
    {
        _nextPickupId = Zones
            .SelectMany(z => z.Pickups)
            .Select(p => p.Id)
            .DefaultIfEmpty(-1)
            .Max() + 1;
    }
}

public class HomePosition
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}