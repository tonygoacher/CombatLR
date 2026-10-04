using CombatLR.mobile.Services;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace CombatLR.mobile.Pages;

public partial class ProjectUpdatePage : ContentPage
{
    private const string FtpUsernameKey = "CombatLR.ProjectUpdate.FtpUsername";
    private const string FtpPasswordKey = "CombatLR.ProjectUpdate.FtpPassword";

    private readonly CombatLRNetworkService _networkService = new();
    private readonly ObservableCollection<FtpDeviceRow> _devices = new();
    private readonly HashSet<string> _completedDeviceIds = new(StringComparer.OrdinalIgnoreCase);
    private string? _projectDirectory;
    private bool _updating;

    public ProjectUpdatePage()
    {
        InitializeComponent();
        DevicesView.ItemsSource = _devices;
        _networkService.DevicesChanged += NetworkService_DevicesChanged;
        _ = LoadCredentialsAsync();
    }

    protected override async void OnAppearing()
    {
        base.OnAppearing();

        await _networkService.StartDiscoveryAsync();
        UpdateDeviceList();
    }

    protected override async void OnDisappearing()
    {
        await _networkService.StopDiscoveryAsync();
        base.OnDisappearing();
    }

    private async Task LoadCredentialsAsync()
    {
        try
        {
            UpdateCredentialsLabel();
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"Project Update: unable to load FTP credentials: {ex.Message}");
        }
    }

    private async void ChooseFolderButton_Clicked(
        object? sender,
        EventArgs e)
    {
        try
        {
            string? documentsDirectory = GetDocumentsDirectory();

            if (string.IsNullOrWhiteSpace(documentsDirectory))
            {
                await DisplayAlert(
                    "Choose Project",
                    "Unable to locate the public Documents folder.",
                    "OK");
                return;
            }

            string combatLRDirectory =
                Path.Combine(documentsDirectory, "CombatLR");

            if (!Directory.Exists(combatLRDirectory))
            {
                await DisplayAlert(
                    "Choose Project",
                    $"The CombatLR project folder does not exist yet:\n\n{combatLRDirectory}",
                    "OK");
                return;
            }

            var projects = Directory.GetDirectories(combatLRDirectory)
                .Where(directory =>
                    File.Exists(Path.Combine(directory, "map.bmp")) &&
                    File.Exists(Path.Combine(directory, "map.json")))
                .OrderBy(directory => directory, StringComparer.OrdinalIgnoreCase)
                .ToArray();

            if (projects.Length == 0)
            {
                await DisplayAlert(
                    "Choose Project",
                    "No valid CombatLR projects were found.\n\nA project must contain map.bmp and map.json.",
                    "OK");
                return;
            }

            string[] names = projects
                .Select(Path.GetFileName)
                .Where(name => !string.IsNullOrWhiteSpace(name))
                .Cast<string>()
                .ToArray();

            string? selected = await DisplayActionSheet(
                "Select Game Plan Folder",
                "Cancel",
                null,
                names);

            if (string.IsNullOrWhiteSpace(selected) ||
                selected == "Cancel")
                return;

            string folder = Path.Combine(combatLRDirectory, selected);

            if (!ValidateProjectFolder(folder, out string error))
            {
                _projectDirectory = null;
                ProjectLabel.Text = "No project selected";
                FolderLabel.Text = "Selected project has no complete game plan";
                UpdateButton.IsEnabled = false;

                await DisplayAlert("Invalid Project", error, "OK");
                return;
            }

            _projectDirectory = folder;
            ProjectLabel.Text = selected;
            FolderLabel.Text = folder;
            UpdateButton.IsEnabled = _devices.Any(row => row.IsSelected && row.Device.IsOnline);
        }
        catch (Exception ex)
        {
            await DisplayAlert("Choose Project", ex.Message, "OK");
        }
    }

    private static string? GetDocumentsDirectory()
    {
#if ANDROID
        return Android.OS.Environment.GetExternalStoragePublicDirectory(
            Android.OS.Environment.DirectoryDocuments)?.AbsolutePath;
#elif WINDOWS
        return Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
#else
        return Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
#endif
    }

    private async void SetCredentialsButton_Clicked(
        object? sender,
        EventArgs e)
    {
        string currentUsername =
            await SecureStorage.Default.GetAsync(FtpUsernameKey) ?? "";

        string? username = await DisplayPromptAsync(
            "FTP Credentials",
            "FTP username:",
            "OK",
            "Cancel",
            "Username",
            100,
            Keyboard.Text,
            currentUsername);

        if (username is null)
            return;

        string currentPassword =
            await SecureStorage.Default.GetAsync(FtpPasswordKey) ?? "";

        string? password = await DisplayPromptAsync(
            "FTP Credentials",
            "FTP password:",
            "OK",
            "Cancel",
            "Password",
            100,
            Keyboard.Default,
            currentPassword);

        if (password is null)
            return;

        username = username.Trim();

        if (string.IsNullOrWhiteSpace(username) ||
            string.IsNullOrEmpty(password))
        {
            await DisplayAlert(
                "FTP Credentials",
                "Both username and password are required.",
                "OK");
            return;
        }

        await SecureStorage.Default.SetAsync(FtpUsernameKey, username);
        await SecureStorage.Default.SetAsync(FtpPasswordKey, password);

        UpdateCredentialsLabel();
    }

    private async void UpdateCredentialsLabel()
    {
        string username =
            await SecureStorage.Default.GetAsync(FtpUsernameKey) ?? "";

        CredentialsLabel.Text =
            string.IsNullOrWhiteSpace(username)
                ? "Not configured"
                : $"Username: {username}   •   Port 21";
    }

    private static bool ValidateProjectFolder(
        string folder,
        out string error)
    {
        string[] requiredFiles =
        {
            "map.bmp",
            "map.json",
            "gameplan.json"
        };

        var missing = requiredFiles
            .Where(file => !File.Exists(Path.Combine(folder, file)))
            .ToArray();

        if (missing.Length != 0)
        {
            error =
                "This project cannot be updated because it is missing:\n\n" +
                string.Join("\n", missing) +
                "\n\nA project update requires map.bmp, map.json and gameplan.json.";
            return false;
        }

        error = "";
        return true;
    }

    private async void DiscoverButton_Clicked(
        object? sender,
        EventArgs e)
    {
        if (_updating)
            return;

        _completedDeviceIds.Clear();
        _devices.Clear();
        UpdateDeviceCount();

        _networkService.ClearDevices();
        await _networkService.StartDiscoveryAsync();
    }

    private void NetworkService_DevicesChanged(
        object? sender,
        EventArgs e)
    {
        MainThread.BeginInvokeOnMainThread(UpdateDeviceList);
    }

    private void UpdateDeviceList()
    {
        if (_updating)
            return;

        var ftpDevices = _networkService.Devices
            .Where(device =>
                device.FtpMode &&
                !_completedDeviceIds.Contains(device.DeviceId))
            .ToDictionary(device => device.DeviceId);

        foreach (var row in _devices.ToArray())
        {
            if (!ftpDevices.ContainsKey(row.Device.DeviceId))
            {
                row.Device.PropertyChanged -= Device_PropertyChanged;
                row.PropertyChanged -= DeviceRow_PropertyChanged;
                _devices.Remove(row);
            }
        }

        foreach (var device in ftpDevices.Values)
        {
            var row = _devices.FirstOrDefault(
                item => item.Device.DeviceId == device.DeviceId);

            if (row is null)
            {
                row = new FtpDeviceRow(device);
                device.PropertyChanged += Device_PropertyChanged;
                row.PropertyChanged += DeviceRow_PropertyChanged;
                _devices.Add(row);
            }

            row.Refresh();
        }

        UpdateDeviceCount();
        UpdateButton.IsEnabled =
            !_updating &&
            _projectDirectory is not null &&
            _devices.Any(row => row.IsSelected && row.Device.IsOnline);
    }

    private void DeviceRow_PropertyChanged(
        object? sender,
        PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(FtpDeviceRow.IsSelected))
        {
            UpdateButton.IsEnabled =
                !_updating &&
                _projectDirectory is not null &&
                _devices.Any(row => row.IsSelected && row.Device.IsOnline);
        }
    }

    private void Device_PropertyChanged(
        object? sender,
        PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(CombatLRDevice.IsOnline) ||
            e.PropertyName == nameof(CombatLRDevice.PlanName) ||
            e.PropertyName == nameof(CombatLRDevice.PlanVersion))
        {
            MainThread.BeginInvokeOnMainThread(UpdateDeviceList);
        }
    }

    private void UpdateDeviceCount()
    {
        int onlineCount = _devices.Count(row => row.Device.IsOnline);
        DevicesHeaderLabel.Text = $"FTP DEVICES: {onlineCount}";
    }

    private async void UpdateButton_Clicked(
        object? sender,
        EventArgs e)
    {
        if (_updating || _projectDirectory is null)
            return;

        if (!ValidateProjectFolder(_projectDirectory, out string validationError))
        {
            await DisplayAlert("Invalid Project", validationError, "OK");
            return;
        }

        string username =
            await SecureStorage.Default.GetAsync(FtpUsernameKey) ?? "";

        string password =
            await SecureStorage.Default.GetAsync(FtpPasswordKey) ?? "";

        if (string.IsNullOrWhiteSpace(username) ||
            string.IsNullOrWhiteSpace(password))
        {
            await DisplayAlert(
                "FTP Credentials",
                "Enter the FTP username and password before updating devices.",
                "OK");
            return;
        }

        var selected = _devices
            .Where(row => row.IsSelected && row.Device.IsOnline)
            .ToList();

        if (selected.Count == 0)
        {
            await DisplayAlert(
                "Project Update",
                "Select at least one online FTP device.",
                "OK");
            return;
        }

        try
        {
            _updating = true;
            UpdateButton.IsEnabled = false;

            await SecureStorage.Default.SetAsync(FtpUsernameKey, username);
            await SecureStorage.Default.SetAsync(FtpPasswordKey, password);

            foreach (var row in selected)
            {
                row.Status = "Updating...";
                row.IsSelected = false;
            }

            string[] files =
            {
                "map.bmp",
                "map.json",
                "gameplan.json"
            };

            foreach (var row in selected)
            {
                bool success = true;

                foreach (string fileName in files)
                {
                    row.Status = $"Uploading {fileName}...";

                    bool uploaded =
                        await _networkService.UploadProjectFileAsync(
                            row.Device,
                            Path.Combine(_projectDirectory, fileName),
                            fileName,
                            username,
                            password);

                    if (!uploaded)
                    {
                        success = false;
                        break;
                    }
                }

                if (success)
                {
                    row.Status = "Updated";
                    _completedDeviceIds.Add(row.Device.DeviceId);
                    row.Device.PropertyChanged -= Device_PropertyChanged;
                    row.PropertyChanged -= DeviceRow_PropertyChanged;
                    _devices.Remove(row);
                }
                else
                {
                    row.Status = "FAILED - retry available";
                    row.IsSelected = true;
                }
            }
        }
        catch (Exception ex)
        {
            await DisplayAlert("Project Update", ex.Message, "OK");
        }
        finally
        {
            _updating = false;
            UpdateDeviceCount();
            UpdateButton.IsEnabled =
                _projectDirectory is not null &&
                _devices.Any(row => row.IsSelected && row.Device.IsOnline);
        }
    }

    private sealed class FtpDeviceRow : INotifyPropertyChanged
    {
        private bool _isSelected;
        private string _status = "Ready";

        public FtpDeviceRow(CombatLRDevice device)
        {
            Device = device;
        }

        public CombatLRDevice Device { get; }

        public bool IsSelected
        {
            get => _isSelected;
            set
            {
                if (_isSelected == value)
                    return;

                _isSelected = value;
                PropertyChanged?.Invoke(
                    this,
                    new PropertyChangedEventArgs(nameof(IsSelected)));
            }
        }

        public string Status
        {
            get => _status;
            set
            {
                if (_status == value)
                    return;

                _status = value;
                PropertyChanged?.Invoke(
                    this,
                    new PropertyChangedEventArgs(nameof(Status)));
            }
        }

        public string PlanDisplay =>
            string.IsNullOrWhiteSpace(Device.PlanName)
                ? $"Plan: v{Device.PlanVersion}"
                : $"Plan: {Device.PlanName}  v{Device.PlanVersion}";

        public event PropertyChangedEventHandler? PropertyChanged;

        public void Refresh()
        {
            PropertyChanged?.Invoke(
                this,
                new PropertyChangedEventArgs(nameof(PlanDisplay)));
        }
    }
}
