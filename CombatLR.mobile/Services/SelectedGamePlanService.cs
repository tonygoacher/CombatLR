using CombatLR.mobile.Models;

namespace CombatLR.mobile.Services;

/// <summary>
/// Holds the single authoritative project, map and game plan selected for the
/// current application session. Pages consume this state rather than opening
/// their own independent project selections.
/// </summary>
public sealed class SelectedGamePlanService
{
    public string? ProjectDirectory { get; private set; }
    public string? ProjectName { get; private set; }
    public GamePlan? GamePlan { get; private set; }
    public MapDefinition? MapDefinition { get; private set; }
    public string? MapBitmapPath { get; private set; }
    public bool IsSelected => ProjectDirectory is not null &&
                              GamePlan is not null &&
                              MapDefinition is not null &&
                              MapBitmapPath is not null;

    public event EventHandler? SelectionChanged;

    public static bool IsValidProjectDirectory(string directory)
    {
        return File.Exists(Path.Combine(directory, "map.bmp")) &&
               File.Exists(Path.Combine(directory, "map.json"));
    }

    public static string? GetProjectsDirectory()
    {
#if ANDROID
        var documents = Android.OS.Environment.GetExternalStoragePublicDirectory(
            Android.OS.Environment.DirectoryDocuments)?.AbsolutePath;
#else
        var documents = Environment.GetFolderPath(
            Environment.SpecialFolder.MyDocuments);
#endif
        return string.IsNullOrWhiteSpace(documents)
            ? null
            : Path.Combine(documents, "CombatLR");
    }

    public async Task SelectProjectAsync(string projectDirectory)
    {
        if (string.IsNullOrWhiteSpace(projectDirectory))
            throw new ArgumentException("A project directory is required.", nameof(projectDirectory));

        var mapPath = Path.Combine(projectDirectory, "map.bmp");
        var mapJsonPath = Path.Combine(projectDirectory, "map.json");
        var gamePlanPath = Path.Combine(projectDirectory, "gameplan.json");

        if (!File.Exists(mapPath))
            throw new FileNotFoundException("The project does not contain map.bmp.", mapPath);

        if (!File.Exists(mapJsonPath))
            throw new FileNotFoundException("The project does not contain map.json.", mapJsonPath);

        await using var jsonStream = File.OpenRead(mapJsonPath);
        var mapDefinition = await System.Text.Json.JsonSerializer.DeserializeAsync<MapDefinition>(
            jsonStream);

        if (mapDefinition is null)
            throw new InvalidDataException("map.json produced no MapDefinition.");

        var planService = new GamePlanService();
        GamePlan gamePlan = File.Exists(gamePlanPath)
            ? await planService.LoadAsync(gamePlanPath)
            : planService.CreateNew();

        var projectName = Path.GetFileName(projectDirectory) ?? string.Empty;
        gamePlan.ProjectName = projectName;

        ProjectDirectory = projectDirectory;
        ProjectName = projectName;
        MapDefinition = mapDefinition;
        MapBitmapPath = mapPath;
        GamePlan = gamePlan;

        SelectionChanged?.Invoke(this, EventArgs.Empty);
    }

    public async Task SaveGamePlanAsync()
    {
        if (!IsSelected || GamePlan is null || ProjectDirectory is null)
            throw new InvalidOperationException("Select a game plan before saving.");

        GamePlan.ProjectName = ProjectName ?? string.Empty;
        var filePath = Path.Combine(ProjectDirectory, "gameplan.json");
        await new GamePlanService().SaveAsync(GamePlan, filePath);
        SelectionChanged?.Invoke(this, EventArgs.Empty);
    }

    /// <summary>
    /// Writes the authoritative in-memory plan to its project folder for FTP
    /// upload, preserving its current version number.
    /// </summary>
    public async Task SaveSelectedGamePlanForUploadAsync()
    {
        if (!IsSelected || GamePlan is null || ProjectDirectory is null)
            throw new InvalidOperationException("Select a game plan before uploading.");

        GamePlan.ProjectName = ProjectName ?? string.Empty;
        var filePath = Path.Combine(ProjectDirectory, "gameplan.json");
        await new GamePlanService().SaveCurrentVersionAsync(GamePlan, filePath);
        SelectionChanged?.Invoke(this, EventArgs.Empty);
    }
}
