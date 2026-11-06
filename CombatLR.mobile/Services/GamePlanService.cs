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

    public Task SaveAsync(GamePlan gamePlan, string filePath)
    {
        return SaveCoreAsync(gamePlan, filePath, incrementVersion: true);
    }

    // Used when preparing the currently selected plan for device upload.
    // Uploading must not itself create a new game plan version.
    public Task SaveCurrentVersionAsync(GamePlan gamePlan, string filePath)
    {
        return SaveCoreAsync(gamePlan, filePath, incrementVersion: false);
    }

    private static async Task SaveCoreAsync(
        GamePlan gamePlan,
        string filePath,
        bool incrementVersion)
    {
        if (gamePlan is null)
            throw new ArgumentNullException(nameof(gamePlan));

        var directory = Path.GetDirectoryName(filePath);
        if (string.IsNullOrWhiteSpace(directory))
            throw new InvalidOperationException("The game plan path has no containing directory.");

        Directory.CreateDirectory(directory);

        var originalVersion = gamePlan.Version;
        var newVersion = incrementVersion
            ? Math.Max(0, originalVersion) + 1
            : originalVersion;

        gamePlan.Version = newVersion;
        var json = JsonSerializer.Serialize(gamePlan, JsonOptions);
        gamePlan.Version = originalVersion;

        var tempPath = filePath + ".tmp";

        try
        {
            await File.WriteAllTextAsync(tempPath, json);

            if (File.Exists(filePath))
                File.Delete(filePath);

            File.Move(tempPath, filePath);
            gamePlan.Version = newVersion;
        }
        catch
        {
            gamePlan.Version = originalVersion;

            if (File.Exists(tempPath))
                File.Delete(tempPath);

            throw;
        }
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
