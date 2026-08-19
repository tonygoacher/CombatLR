namespace CombatLR.mobile.Models;

public enum PickupType
{
    Health = 0,
    Reload = 1,
    WeaponBonus = 2
}

public class GamePickup
{
    // Position relative to Home, in metres.
    public double EastMetres { get; set; }
    public double NorthMetres { get; set; }

    // Type of pickup.
    // Stored as an integer in JSON.
    public PickupType Type { get; set; }

    // Seconds after the containing zone starts.
    public int SpawnTimeSeconds { get; set; }

    // Seconds the pickup remains active.
    // 0 = permanent.
    public int LifespanSeconds { get; set; }

    public int Id { get; set; }
}