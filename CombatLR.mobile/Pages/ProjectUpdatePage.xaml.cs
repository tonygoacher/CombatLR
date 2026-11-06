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
    private readonly HashSet<string> _completedDeviceIds =
        new(StringComparer.OrdinalIgnoreCase);

    private string? _projectDirectory;
    private readonly SelectedGamePlanService _selectedGamePlan;

    private bool _updating;
    private bool _cancelUpdate;
    private bool _leaving;

    private Task? _updateTask;

    public ProjectUpdatePage(SelectedGamePlanService selectedGamePlan)
    {
        InitializeComponent();
        _selectedGamePlan = selectedGamePlan;

        DevicesView.ItemsSource = _devices;

        _networkService.DevicesChanged += NetworkService_DevicesChanged;

        _ = LoadCredentialsAsync();
    }

    protected override async void OnAppearing()
    {
        base.OnAppearing();

        _leaving = false;

        UpdateSelectedProjectDisplay();

        await _networkService.StartDiscoveryAsync();

        UpdateDeviceList();
    }

    protected override async void OnDisappearing()
    {
        await _networkService.StopDiscoveryAsync();

        base.OnDisappearing();
    }

    // -------------------------------------------------------------------------
    // Back button
    // -------------------------------------------------------------------------

    protected override bool OnBackButtonPressed()
    {
        if (!_updating || _leaving)
            return base.OnBackButtonPressed();

        _leaving = true;
        _cancelUpdate = true;

        _ = CancelAndLeaveAsync();

        return true;
    }

    private async Task CancelAndLeaveAsync()
    {
        try
        {
            /*
             * Do not abort an FTP transfer in the middle of a file.
             *
             * UploadProjectFileAsync() does not currently accept a
             * CancellationToken, so allow the current transfer to complete.
             * The update loop will then stop before starting another file
             * or another device.
             */
            if (_updateTask is not null)
            {
                await _updateTask;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"Project Update: cancellation cleanup: {ex.Message}");
        }

        try
        {
            await MainThread.InvokeOnMainThreadAsync(
                async () =>
                {
                    await Navigation.PopAsync();
                });
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"Project Update: unable to leave page: {ex.Message}");
        }
    }

    // -------------------------------------------------------------------------
    // Credentials
    // -------------------------------------------------------------------------

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

    private async void SetCredentialsButton_Clicked(
        object? sender,
        EventArgs e)
    {
        if (_updating)
            return;

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

        await SecureStorage.Default.SetAsync(
            FtpUsernameKey,
            username);

        await SecureStorage.Default.SetAsync(
            FtpPasswordKey,
            password);

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

    // -------------------------------------------------------------------------
    // Selected project
    // -------------------------------------------------------------------------

    private void UpdateSelectedProjectDisplay()
    {
        _projectDirectory = null;

        if (!_selectedGamePlan.IsSelected ||
            _selectedGamePlan.ProjectDirectory is null ||
            _selectedGamePlan.GamePlan is null)
        {
            ProjectLabel.Text = "No game plan selected";
            ProjectVersionLabel.Text = "Version: —";
            FolderLabel.Text = "Return to Game Plan Selection and choose a project.";
            UpdateButton.IsEnabled = false;
            return;
        }

        ProjectLabel.Text = _selectedGamePlan.ProjectName ?? "Selected project";
        ProjectVersionLabel.Text =
            $"Version: {_selectedGamePlan.GamePlan.Version}";
        FolderLabel.Text = _selectedGamePlan.ProjectDirectory;

        if (!ValidateProjectFolder(
                _selectedGamePlan.ProjectDirectory,
                out string error))
        {
            FolderLabel.Text =
                $"{_selectedGamePlan.ProjectDirectory}\n{error}";
            UpdateButton.IsEnabled = false;
            return;
        }

        _projectDirectory = _selectedGamePlan.ProjectDirectory;
        UpdateButton.IsEnabled =
            !_updating &&
            _devices.Any(row => row.IsSelected && row.Device.IsOnline);
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
                "\n\nSave the game plan in the editor before updating devices.";

            return false;
        }

        error = "";
        return true;
    }

    // -------------------------------------------------------------------------
    // Discovery
    // -------------------------------------------------------------------------

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
        MainThread.BeginInvokeOnMainThread(
            UpdateDeviceList);
    }

    private void UpdateDeviceList()
    {
        /*
         * Do not allow discovery updates to modify the collection while
         * an FTP update is in progress.
         */
        if (_updating)
            return;

        var ftpDevices = _networkService.Devices
            .Where(device =>
                device.FtpMode &&
                !_completedDeviceIds.Contains(device.DeviceId))
            .ToDictionary(
                device => device.DeviceId);

        foreach (var row in _devices.ToArray())
        {
            if (!ftpDevices.ContainsKey(row.Device.DeviceId))
            {
                row.Device.PropertyChanged -=
                    Device_PropertyChanged;

                row.PropertyChanged -=
                    DeviceRow_PropertyChanged;

                _devices.Remove(row);
            }
        }

        foreach (var device in ftpDevices.Values)
        {
            var row = _devices.FirstOrDefault(
                item =>
                    item.Device.DeviceId ==
                    device.DeviceId);

            if (row is null)
            {
                row = new FtpDeviceRow(device);

                device.PropertyChanged +=
                    Device_PropertyChanged;

                row.PropertyChanged +=
                    DeviceRow_PropertyChanged;

                _devices.Add(row);
            }

            row.Refresh();
        }

        UpdateDeviceCount();

        UpdateButton.IsEnabled =
            !_updating &&
            _projectDirectory is not null &&
            _devices.Any(
                row =>
                    row.IsSelected &&
                    row.Device.IsOnline);
    }

    private void DeviceRow_PropertyChanged(
        object? sender,
        PropertyChangedEventArgs e)
    {
        if (e.PropertyName ==
            nameof(FtpDeviceRow.IsSelected))
        {
            UpdateButton.IsEnabled =
                !_updating &&
                _projectDirectory is not null &&
                _devices.Any(
                    row =>
                        row.IsSelected &&
                        row.Device.IsOnline);
        }
    }

    private void Device_PropertyChanged(
        object? sender,
        PropertyChangedEventArgs e)
    {
        if (e.PropertyName ==
                nameof(CombatLRDevice.IsOnline) ||
            e.PropertyName ==
                nameof(CombatLRDevice.PlanName) ||
            e.PropertyName ==
                nameof(CombatLRDevice.PlanVersion))
        {
            MainThread.BeginInvokeOnMainThread(
                UpdateDeviceList);
        }
    }

    private void UpdateDeviceCount()
    {
        int onlineCount =
            _devices.Count(
                row => row.Device.IsOnline);

        DevicesHeaderLabel.Text =
            $"FTP DEVICES: {onlineCount}";
    }

    // -------------------------------------------------------------------------
    // Project update
    // -------------------------------------------------------------------------

    private async void UpdateButton_Clicked(
        object? sender,
        EventArgs e)
    {
        if (_updating ||
            _projectDirectory is null)
        {
            return;
        }

        if (!ValidateProjectFolder(
                _projectDirectory,
                out string validationError))
        {
            await DisplayAlert(
                "Invalid Project",
                validationError,
                "OK");

            return;
        }

        string username =
            await SecureStorage.Default.GetAsync(
                FtpUsernameKey) ?? "";

        string password =
            await SecureStorage.Default.GetAsync(
                FtpPasswordKey) ?? "";

        if (string.IsNullOrWhiteSpace(username) ||
            string.IsNullOrWhiteSpace(password))
        {
            await DisplayAlert(
                "FTP Credentials",
                "Enter the FTP username and password " +
                "before updating devices.",
                "OK");

            return;
        }

        var selected = _devices
            .Where(
                row =>
                    row.IsSelected &&
                    row.Device.IsOnline)
            .ToList();

        if (selected.Count == 0)
        {
            await DisplayAlert(
                "Project Update",
                "Select at least one online FTP device.",
                "OK");

            return;
        }

        // gameplan.json is generated from the shared in-memory selection,
        // not assumed to be an up-to-date copy left on disk by the editor.
        try
        {
            await _selectedGamePlan.SaveSelectedGamePlanForUploadAsync();
            ProjectVersionLabel.Text =
                $"Version: {_selectedGamePlan.GamePlan?.Version}";
        }
        catch (Exception ex)
        {
            await DisplayAlert(
                "Game Plan",
                $"The selected game plan could not be prepared for upload. No devices have been updated.\n\n{ex.Message}",
                "OK");
            return;
        }

        _cancelUpdate = false;

        /*
         * Capture the task so the Back button can wait for the current
         * FTP operation to finish cleanly.
         */
        _updateTask = UpdateSelectedDevicesAsync(
            selected,
            username,
            password);

        try
        {
            await _updateTask;
        }
        finally
        {
            _updateTask = null;
        }
    }

    private async Task UpdateSelectedDevicesAsync(
        List<FtpDeviceRow> selected,
        string username,
        string password)
    {
        try
        {
            _updating = true;

            UpdateButton.IsEnabled = false;

            await SecureStorage.Default.SetAsync(
                FtpUsernameKey,
                username);

            await SecureStorage.Default.SetAsync(
                FtpPasswordKey,
                password);

            foreach (var row in selected)
            {
                row.Status = "Updating...";

            }

            string[] files =
            {
                "map.bmp",
                "map.json",
                "gameplan.json"
            };

            foreach (var row in selected)
            {
                /*
                 * Back was pressed while processing a previous device. */
                bool success = true;
                if (_cancelUpdate)
                {
                    success = false;
                    row.Status = "Cancelled";
                    row.Progress = 0;
                    break;
                }


                foreach (string fileName in files)
                {
                    /*
                     * Never start another FTP transfer once cancellation
                     * has been requested.
                     */
                    if (_cancelUpdate)
                    {
                        success = false;

                        row.Status = "Cancelled";
                        row.Progress = 0;
                        row.IsSelected = true;

                        break;
                    }

                    row.Progress = 0;
                    row.Status =
                        $"Uploading {fileName}... 0%";

                    var progress =
                        new Progress<FtpTransferProgress>(update =>
                        {
                            row.Progress = update.Percentage / 100.0;
                            row.Status =
                                $"Uploading {update.FileName}... {update.Percentage}%";
                        });

                    bool uploaded =
                        await _networkService
                            .UploadProjectFileAsync(
                                row.Device,
                                Path.Combine(
                                    _projectDirectory!,
                                    fileName),
                                fileName,
                                username,
                                password,
                                progress: progress);

                    /*
                     * If Back was pressed during the transfer,
                     * UploadProjectFileAsync() is allowed to finish.
                     *
                     * We now stop processing this device rather than
                     * starting another file.
                     */
                    if (_cancelUpdate)
                    {
                        success = false;

                        row.Status = "Cancelled";
                        row.Progress = 0;
                        row.IsSelected = true;

                        break;
                    }

                    if (!uploaded)
                    {
                        success = false;

                        break;
                    }
                }

                if (_cancelUpdate)
                {
                    row.Status = "Cancelled";
                    row.IsSelected = true;

                    continue;
                }

                if (success)
                {
                    row.Status = "Updated";
                    row.Progress = 1.0;

                    _completedDeviceIds.Add(
                        row.Device.DeviceId);

                    row.Device.PropertyChanged -=
                        Device_PropertyChanged;

                    row.PropertyChanged -=
                        DeviceRow_PropertyChanged;

                    _devices.Remove(row);
                }
                else
                {
                    row.Status =
                        "FAILED - retry available";
                    row.Progress = 0;

                    row.IsSelected = true;
                }
            }
        }
        catch (Exception ex)
        {
            /*
             * If the user is leaving the page, don't display an alert
             * over the page that is about to disappear.
             */
            if (!_cancelUpdate &&
                !_leaving)
            {
                await MainThread.InvokeOnMainThreadAsync(
                    async () =>
                    {
                        await DisplayAlert(
                            "Project Update",
                            ex.Message,
                            "OK");
                    });
            }
            else
            {
                Console.WriteLine(
                    $"Project Update: {ex.Message}");
            }
        }
        finally
        {
            _updating = false;

            if (!_leaving)
            {
                UpdateDeviceCount();

                UpdateButton.IsEnabled =
                    _projectDirectory is not null &&
                    _devices.Any(
                        row =>
                            row.IsSelected &&
                            row.Device.IsOnline);
            }
        }
    }

    // -------------------------------------------------------------------------
    // FTP device row
    // -------------------------------------------------------------------------

    private sealed class FtpDeviceRow :
        INotifyPropertyChanged
    {
        private bool _isSelected;
        private string _status = "Ready";
        private double _progress;

        public FtpDeviceRow(
            CombatLRDevice device)
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
                    new PropertyChangedEventArgs(
                        nameof(IsSelected)));
            }
        }

        public double Progress
        {
            get => _progress;
            set
            {
                if (Math.Abs(_progress - value) < 0.0001)
                    return;

                _progress = value;

                PropertyChanged?.Invoke(
                    this,
                    new PropertyChangedEventArgs(
                        nameof(Progress)));
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
                    new PropertyChangedEventArgs(
                        nameof(Status)));
            }
        }

        public string PlanDisplay =>
            string.IsNullOrWhiteSpace(
                Device.PlanName)
                ? $"Plan: v{Device.PlanVersion}"
                : $"Plan: {Device.PlanName}  " +
                  $"v{Device.PlanVersion}";

        public event PropertyChangedEventHandler?
            PropertyChanged;

        public void Refresh()
        {
            PropertyChanged?.Invoke(
                this,
                new PropertyChangedEventArgs(
                    nameof(PlanDisplay)));
        }
    }
}