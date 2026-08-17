using System.Text.Json;
using CombatLR.mobile.Models;

namespace CombatLR.mobile.Services;

public class GamePlanService
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase
    };

    public async Task SaveAsync(
        GamePlan gamePlan,
        string filePath)
    {
        await using var stream =
            File.Create(filePath);

        await JsonSerializer.SerializeAsync(
            stream,
            gamePlan,
            JsonOptions);
    }

    public async Task<GamePlan> LoadAsync(
        string filePath)
    {
        await using var stream =
            File.OpenRead(filePath);

        var gamePlan =
            await JsonSerializer.DeserializeAsync<GamePlan>(
                stream,
                JsonOptions);

        if (gamePlan is null)
            throw new InvalidDataException(
                "Game plan JSON contained no GamePlan.");

        return gamePlan;
    }

    public GamePlan CreateNew()
    {
        return new GamePlan();
    }
}