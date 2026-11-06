using System.Globalization;
using System.Text.Json;

namespace CombatLR.mobile.Services;

/// <summary>
/// Persists the most recently acknowledged game start. A session is valid only
/// for the project name and plan version that were active when START GAME was
/// acknowledged.
/// </summary>
public sealed class GameSessionService
{
    private const string PreferenceKey = "CombatLR.LastGameSession";

    public GameSession? GetSession(string? projectName, int planVersion)
    {
        if (string.IsNullOrWhiteSpace(projectName))
            return null;

        try
        {
            var json = Preferences.Default.Get(PreferenceKey, "");
            if (string.IsNullOrWhiteSpace(json))
                return null;

            var session = JsonSerializer.Deserialize<GameSession>(json);
            if (session is null ||
                session.PlanVersion != planVersion ||
                !string.Equals(session.ProjectName, projectName,
                    StringComparison.OrdinalIgnoreCase))
            {
                return null;
            }

            return session;
        }
        catch (JsonException)
        {
            return null;
        }
    }

    public GameSession RecordStart(
        string projectName,
        int planVersion,
        string gameStart)
    {
        if (!TimeSpan.TryParseExact(
                gameStart,
                @"hh\:mm\:ss",
                CultureInfo.InvariantCulture,
                out var timeOfDay))
        {
            throw new FormatException($"Invalid game start time: {gameStart}");
        }

        // The firmware protocol carries UTC time-of-day only. Resolve it to
        // the nearest UTC date so starts around midnight remain unambiguous.
        var now = DateTime.UtcNow;
        var candidate = DateTime.SpecifyKind(
            now.Date.Add(timeOfDay),
            DateTimeKind.Utc);

        if ((candidate - now).TotalHours > 12)
            candidate = candidate.AddDays(-1);
        else if ((now - candidate).TotalHours > 12)
            candidate = candidate.AddDays(1);

        var session = new GameSession
        {
            ProjectName = projectName,
            PlanVersion = planVersion,
            StartedAtUtc = new DateTimeOffset(candidate, TimeSpan.Zero)
        };

        Preferences.Default.Set(
            PreferenceKey,
            JsonSerializer.Serialize(session));

        return session;
    }
}

public sealed class GameSession
{
    public string ProjectName { get; set; } = "";
    public int PlanVersion { get; set; }
    public DateTimeOffset StartedAtUtc { get; set; }
}
