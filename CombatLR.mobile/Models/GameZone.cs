namespace CombatLR.mobile.Models;

public class GameZone
{
    public string Id { get; set; } = Guid.NewGuid().ToString();

    // Position relative to Home, in metres.
    // East = positive eastwards.
    // North = positive northwards.
    public double East { get; set; }

    public double North { get; set; }

    // Zone radius in metres.
    public double Radius { get; set; } = 50.0;

    // Seconds from game start.
    public int StartTime { get; set; }
}