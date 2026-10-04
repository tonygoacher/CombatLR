using CombatLR.mobile.Models;
using System.Text.Json;

namespace CombatLR.mobile.Services;

public class GamePlanService
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase
    };

    public GamePlan CreateNew()
    {
        // Version zero is an unsaved game plan. The first successful save
        // becomes version 1.
        return new GamePlan
        {
            Version = 0
        };
    }

    public async Task SaveAsync(GamePlan gamePlan, string filePath)
    {
        if (gamePlan is null)
            throw new ArgumentNullException(nameof(gamePlan));

        var directory = Path.GetDirectoryName(filePath);
        if (string.IsNullOrWhiteSpace(directory))
            throw new InvalidOperationException("The game plan path has no containing directory.");

        Directory.CreateDirectory(directory);

        // Increment only when a save is actually attempted. The new version
        // is written atomically so a failed write does not leave a partial JSON file.
        var newVersion = Math.Max(0, gamePlan.Version) + 1;
        var originalVersion = gamePlan.Version;
        gamePlan.Version = newVersion;
        var json = JsonSerializer.Serialize(gamePlan, JsonOptions);
        gamePlan.Version = originalVersion;

        var tempPath = filePath + ".tmp";

        await File.WriteAllTextAsync(tempPath, json);

        if (File.Exists(filePath))
            File.Delete(filePath);

        File.Move(tempPath, filePath);
        gamePlan.Version = newVersion;
    }

    public async Task<GamePlan> LoadAsync(string filePath)
    {
        await using var stream = File.OpenRead(filePath);

        var gamePlan = await JsonSerializer.DeserializeAsync<GamePlan>(
            stream,
            JsonOptions);

        if (gamePlan is null)
            throw new InvalidDataException("gameplan.json produced no GamePlan.");

        // Older game plans may not have had a version. They remain loadable;
        // their first save will establish version 1.
        if (gamePlan.Version < 0)
            gamePlan.Version = 0;

        gamePlan.InitialisePickupIdAllocator();
        return gamePlan;
    }
}
