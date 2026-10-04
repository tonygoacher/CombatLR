using System.Text.Json.Serialization;

namespace CombatLR.mobile.Models;

public class GamePlan
{
    // 0 means this plan has never been saved. SaveAsync changes it to 1
    // on the first successful save and increments it thereafter.
    public int Version { get; set; } = 0;

    // ProjectName is the name of the containing CombatLR project directory.
    public string ProjectName { get; set; } = string.Empty;

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
