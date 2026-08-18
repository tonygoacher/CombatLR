namespace CombatLR.mobile.Models;

public class GameZone
{
    // Position relative to Home, in metres.
    public double EastMetres { get; set; }
    public double NorthMetres { get; set; }

    // Zone radius in metres.
    public double RadiusMetres { get; set; }

    // Time from the start of the game when this zone becomes active.
    public int StartTime { get; set; }

    // How long the previous zone remains active after
    // this zone starts.
    public int TransitionTime { get; set; }
}