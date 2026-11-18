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

    // Maximum distance from the player at which a pickup can be collected.
    [JsonPropertyName("pickupRadius")]
    public float PickupRadius { get; set; } = 2.0f;

    public List<ZonePickupPercentage> ZonePercentages { get; set; } = new()
    {
        new ZonePickupPercentage { Zone = 0, AllocationPercentage = 40, HealthPercentage = 40, AmmoPercentage = 40, WeaponPercentage = 20 }
    };

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

public class ZonePickupPercentage
{
    public int Zone { get; set; }

    // Percentage of candidate pickup locations retained by the firmware.
    // Defaults to the legacy fixed 40% allocation for existing game plans.
    public int AllocationPercentage { get; set; } = 40;

    public int HealthPercentage { get; set; } = 40;
    public int AmmoPercentage { get; set; } = 40;
    public int WeaponPercentage { get; set; } = 20;
}

public class HomePosition
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}
