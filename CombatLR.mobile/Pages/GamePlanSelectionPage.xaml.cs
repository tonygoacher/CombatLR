using CombatLR.mobile.Services;

namespace CombatLR.mobile.Pages;

public partial class GamePlanSelectionPage : ContentPage
{
    private readonly SelectedGamePlanService _selectedPlan;
    private readonly List<ProjectOption> _projects = new();

    public GamePlanSelectionPage(SelectedGamePlanService selectedPlan)
    {
        InitializeComponent();
        _selectedPlan = selectedPlan;
        ProjectPicker.ItemsSource = _projects;
        RefreshProjects();
    }

    protected override void OnAppearing()
    {
        base.OnAppearing();
        RefreshProjects();
    }

    private void RefreshProjects()
    {
        try
        {
#if ANDROID
            if (Android.OS.Build.VERSION.SdkInt >= Android.OS.BuildVersionCodes.R &&
                !Android.OS.Environment.IsExternalStorageManager)
            {
                PlanInfoLabel.Text =
                    "Android storage access is required. Enable “Allow access to manage all files” for CombatLR, then refresh.";
                RefreshButton.IsEnabled = true;
                ContinueButton.IsEnabled = false;
                RenameProjectButton.IsEnabled = false;
                return;
            }
#endif

            var root = SelectedGamePlanService.GetProjectsDirectory();
            if (string.IsNullOrWhiteSpace(root) || !Directory.Exists(root))
            {
                PlanInfoLabel.Text = root is null
                    ? "The Documents folder could not be located."
                    : $"No CombatLR projects folder was found at:\n{root}";
                RefreshButton.IsEnabled = true;
                ContinueButton.IsEnabled = false;
                RenameProjectButton.IsEnabled = false;
                return;
            }

            _projects.Clear();
            _projects.AddRange(Directory.GetDirectories(root)
                .Where(SelectedGamePlanService.IsValidProjectDirectory)
                .Select(projectPath => new ProjectOption(
                    Path.GetFileName(projectPath) ?? projectPath, projectPath))
                .OrderBy(project => project.Name, StringComparer.OrdinalIgnoreCase));

            ProjectPicker.ItemsSource = null;
            ProjectPicker.ItemsSource = _projects;

            if (_selectedPlan.ProjectDirectory is not null)
            {
                ProjectPicker.SelectedItem = _projects.FirstOrDefault(
                    project => PathsEqual(project.Directory, _selectedPlan.ProjectDirectory));
            }

            PlanInfoLabel.Text = _projects.Count == 0
                ? "No valid projects found. Each project must contain map.bmp and map.json."
                : $"{_projects.Count} project(s) available.";
            UpdateSelectionButtons();
            RefreshButton.IsEnabled = true;
        }
        catch (Exception ex)
        {
            PlanInfoLabel.Text = $"Could not discover projects:\n{ex.Message}";
            ContinueButton.IsEnabled = false;
            RenameProjectButton.IsEnabled = false;
            RefreshButton.IsEnabled = true;
        }
    }

    private void ProjectPicker_SelectedIndexChanged(object? sender, EventArgs e)
    {
        UpdateSelectionButtons();
    }

    private void UpdateSelectionButtons()
    {
        bool hasSelection = ProjectPicker.SelectedItem is ProjectOption;
        ContinueButton.IsEnabled = hasSelection;
        RenameProjectButton.IsEnabled = hasSelection;
    }

    private async void ContinueButton_Clicked(object? sender, EventArgs e)
    {
        if (ProjectPicker.SelectedItem is not ProjectOption selected)
            return;

        ContinueButton.IsEnabled = false;
        RenameProjectButton.IsEnabled = false;
        RefreshButton.IsEnabled = false;
        try
        {
            PlanInfoLabel.Text = $"Loading {selected.Name}...";
            await _selectedPlan.SelectProjectAsync(selected.Directory);
            await Navigation.PushAsync(new MainPage(_selectedPlan));
        }
        catch (Exception ex)
        {
            await DisplayAlert("Game Plan", ex.Message, "OK");
            PlanInfoLabel.Text = "The selected project could not be loaded.";
        }
        finally
        {
            UpdateSelectionButtons();
            RefreshButton.IsEnabled = true;
        }
    }

    private async void RenameProjectButton_Clicked(object? sender, EventArgs e)
    {
        if (ProjectPicker.SelectedItem is not ProjectOption selected)
            return;

        var requestedName = await DisplayPromptAsync(
            "Rename Project",
            $"Enter a new directory name for “{selected.Name}”.",
            accept: "Rename",
            cancel: "Cancel",
            placeholder: "Project name",
            maxLength: 100,
            keyboard: Keyboard.Text,
            initialValue: selected.Name);

        if (requestedName is null)
            return;

        if (!TryValidateProjectName(
                requestedName,
                selected.Directory,
                SelectedGamePlanService.GetProjectsDirectory(),
                out var targetDirectory,
                out var validationError))
        {
            await DisplayAlert("Invalid Project Name", validationError, "OK");
            return;
        }

        if (targetDirectory is null ||
            PathsEqual(selected.Directory, targetDirectory))
        {
            PlanInfoLabel.Text = "The project name has not changed.";
            return;
        }

        RenameProjectButton.IsEnabled = false;
        ContinueButton.IsEnabled = false;
        RefreshButton.IsEnabled = false;

        try
        {
            Directory.Move(selected.Directory, targetDirectory);

            // Keep any already-loaded selection in sync with the moved folder.
            _selectedPlan.OnProjectDirectoryRenamed(
                selected.Directory,
                targetDirectory);

            RefreshProjects();

            // RefreshProjects may restore a previously selected project. The
            // folder just renamed must remain selected for the Continue action.
            ProjectPicker.SelectedItem = _projects.FirstOrDefault(
                project => PathsEqual(project.Directory, targetDirectory));

            UpdateSelectionButtons();
            PlanInfoLabel.Text =
                $"Project renamed to “{Path.GetFileName(targetDirectory)}”.";
        }
        catch (Exception ex)
        {
            UpdateSelectionButtons();
            await DisplayAlert(
                "Rename Failed",
                $"The project directory could not be renamed.\n\n{ex.Message}",
                "OK");
        }
        finally
        {
            RefreshButton.IsEnabled = true;
            UpdateSelectionButtons();
        }
    }

    private static bool TryValidateProjectName(
        string requestedName,
        string sourceDirectory,
        string? projectsRoot,
        out string? targetDirectory,
        out string error)
    {
        targetDirectory = null;
        error = string.Empty;

        var name = requestedName.Trim();
        if (string.IsNullOrWhiteSpace(name))
        {
            error = "A project name cannot be empty.";
            return false;
        }

        if (name is "." or "..")
        {
            error = "The project name cannot be “.” or “..”.";
            return false;
        }

        // Apply Windows filename restrictions on every platform so projects
        // can be moved between Android, Windows and other supported devices.
        const string crossPlatformInvalidCharacters = "<>:\"/\\|?*";
        if (name.Any(character =>
                character < 32 ||
                crossPlatformInvalidCharacters.Contains(character) ||
                Path.GetInvalidFileNameChars().Contains(character)))
        {
            error = "The project name contains a character that is not allowed in a directory name.";
            return false;
        }

        if (name.EndsWith(' ') || name.EndsWith('.'))
        {
            error = "A project name cannot end with a space or a period.";
            return false;
        }

        var deviceName = name.Split('.')[0].TrimEnd(' ', '.');
        var reservedNames = new[]
        {
            "CON", "PRN", "AUX", "NUL",
            "COM1", "COM2", "COM3", "COM4", "COM5",
            "COM6", "COM7", "COM8", "COM9",
            "LPT1", "LPT2", "LPT3", "LPT4", "LPT5",
            "LPT6", "LPT7", "LPT8", "LPT9"
        };

        if (reservedNames.Contains(deviceName, StringComparer.OrdinalIgnoreCase))
        {
            error = "That name is reserved by Windows and cannot be used for a project directory.";
            return false;
        }

        if (string.IsNullOrWhiteSpace(projectsRoot))
        {
            error = "The CombatLR projects directory could not be located.";
            return false;
        }

        try
        {
            var rootPath = Path.TrimEndingDirectorySeparator(
                Path.GetFullPath(projectsRoot));
            var sourcePath = Path.GetFullPath(sourceDirectory);
            var sourceParent = Path.GetDirectoryName(sourcePath);

            if (sourceParent is null || !PathsEqual(sourceParent, rootPath))
            {
                error = "The selected project is not directly inside the CombatLR projects directory.";
                return false;
            }

            var destinationPath = Path.GetFullPath(Path.Combine(rootPath, name));
            var destinationParent = Path.GetDirectoryName(destinationPath);

            if (destinationParent is null || !PathsEqual(destinationParent, rootPath))
            {
                error = "The project name must be a single directory name, not a path.";
                return false;
            }

            if (!PathsEqual(sourcePath, destinationPath) &&
                (Directory.Exists(destinationPath) || File.Exists(destinationPath)))
            {
                error = "A file or project directory with that name already exists.";
                return false;
            }

            targetDirectory = destinationPath;
            return true;
        }
        catch (Exception ex) when (
            ex is ArgumentException ||
            ex is NotSupportedException ||
            ex is PathTooLongException)
        {
            error = $"The project name is not valid. {ex.Message}";
            return false;
        }
    }

    private static bool PathsEqual(string first, string second)
    {
        try
        {
            return string.Equals(
                Path.TrimEndingDirectorySeparator(Path.GetFullPath(first)),
                Path.TrimEndingDirectorySeparator(Path.GetFullPath(second)),
                StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    private async void RefreshButton_Clicked(object? sender, EventArgs e)
    {
#if ANDROID
        if (Android.OS.Build.VERSION.SdkInt >= Android.OS.BuildVersionCodes.R &&
            !Android.OS.Environment.IsExternalStorageManager)
        {
            var packageUri = Android.Net.Uri.Parse(
                $"package:{Android.App.Application.Context.PackageName}");
            var settingsIntent = new Android.Content.Intent(
                Android.Provider.Settings.ActionManageAppAllFilesAccessPermission,
                packageUri);
            var activity = Microsoft.Maui.ApplicationModel.Platform.CurrentActivity;

            if (activity is null)
            {
                await DisplayAlert("Storage Access Required",
                    "CombatLR could not access the current Android activity. Please try again.",
                    "OK");
                return;
            }

            activity.StartActivity(settingsIntent);
            await DisplayAlert("Storage Access Required",
                "Android requires CombatLR to access project files in Documents/CombatLR. " +
                "Enable “Allow access to manage all files” for CombatLR, return to the app, " +
                "then press Refresh Projects.",
                "OK");
        }
#endif
        RefreshProjects();
    }

    private sealed record ProjectOption(string Name, string Directory)
    {
        public override string ToString() => Name;
    }
}
