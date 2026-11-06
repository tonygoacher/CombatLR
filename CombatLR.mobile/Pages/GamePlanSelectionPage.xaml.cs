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
#if ANDROID
        try
        {
            if (Android.OS.Build.VERSION.SdkInt >= Android.OS.BuildVersionCodes.R &&
                !Android.OS.Environment.IsExternalStorageManager)
            {
                PlanInfoLabel.Text =
                    "Android storage access is required. Enable “Allow access to manage all files” for CombatLR, then refresh.";
                RefreshButton.IsEnabled = true;
                ContinueButton.IsEnabled = false;
                return;
            }

            var root = SelectedGamePlanService.GetProjectsDirectory();
            if (string.IsNullOrWhiteSpace(root) || !Directory.Exists(root))
            {
                PlanInfoLabel.Text = root is null
                    ? "The Documents folder could not be located."
                    : $"No CombatLR projects folder was found at:\n{root}";
                RefreshButton.IsEnabled = true;
                ContinueButton.IsEnabled = false;
                return;
            }

            _projects.Clear();
            _projects.AddRange(Directory.GetDirectories(root)
                .Where(SelectedGamePlanService.IsValidProjectDirectory)
                .Select(path => new ProjectOption(
                    Path.GetFileName(path) ?? path, path))
                .OrderBy(project => project.Name, StringComparer.OrdinalIgnoreCase));

            ProjectPicker.ItemsSource = null;
            ProjectPicker.ItemsSource = _projects;

            if (_selectedPlan.ProjectDirectory is not null)
            {
                ProjectPicker.SelectedItem = _projects.FirstOrDefault(
                    p => string.Equals(p.Directory, _selectedPlan.ProjectDirectory,
                        StringComparison.OrdinalIgnoreCase));
            }

            PlanInfoLabel.Text = _projects.Count == 0
                ? "No valid projects found. Each project must contain map.bmp and map.json."
                : $"{_projects.Count} project(s) available.";
            ContinueButton.IsEnabled = ProjectPicker.SelectedItem is not null;
            RefreshButton.IsEnabled = true;
        }
        catch (Exception ex)
        {
            PlanInfoLabel.Text = $"Could not discover projects:\n{ex.Message}";
            ContinueButton.IsEnabled = false;
        }
#else
        PlanInfoLabel.Text =
            "Project discovery is currently implemented for Android.";
        RefreshButton.IsEnabled = true;
        ContinueButton.IsEnabled = false;
#endif
    }

    private void ProjectPicker_SelectedIndexChanged(object? sender, EventArgs e)
    {
        ContinueButton.IsEnabled = ProjectPicker.SelectedItem is not null;
    }

    private async void ContinueButton_Clicked(object? sender, EventArgs e)
    {
        if (ProjectPicker.SelectedItem is not ProjectOption selected)
            return;

#if ANDROID
        ContinueButton.IsEnabled = false;
        RefreshButton.IsEnabled = false;
        try
        {
            PlanInfoLabel.Text = $"Loading {selected.Name}...";
            await _selectedPlan.SelectProjectAsync(selected.Directory);
            await Navigation.PushAsync(new MainPage());
        }
        catch (Exception ex)
        {
            await DisplayAlert("Game Plan", ex.Message, "OK");
            PlanInfoLabel.Text = "The selected project could not be loaded.";
        }
        finally
        {
            ContinueButton.IsEnabled = ProjectPicker.SelectedItem is not null;
            RefreshButton.IsEnabled = true;
        }
#else
        await DisplayAlert("Game Plan",
            "Project discovery is currently implemented for Android.", "OK");
#endif
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
