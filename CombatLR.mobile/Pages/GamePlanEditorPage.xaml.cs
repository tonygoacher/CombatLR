using CombatLR.mobile.Models;
using CombatLR.mobile.Services;
using Microsoft.Maui.Controls.Shapes;
using System.Diagnostics;
using System.Security.Cryptography.X509Certificates;
using System.Text.Json;

#if ANDROID
[assembly: Android.App.UsesPermission(Android.Manifest.Permission.ManageExternalStorage)]
#endif

namespace CombatLR.mobile.Pages
{
    public partial class GamePlanEditorPage : ContentPage
    {
        private MapDefinition? _mapDefinition;
        private MapCoordinateService? _coordinateService;
        private readonly LocationService _locationService;

        private CoordinateInputMode _coordinateInputMode =
        CoordinateInputMode.Tap;

        private enum CoordinateOperation
        {
            None,
            SetHome,
            AddZone,
            MoveZone,
            AddPickup,
        }

        private CoordinateOperation _coordinateOperation =
                    CoordinateOperation.None;

        private enum CoordinateInputMode
        {
            Tap,
            Gps
        }

        private bool _acquiringCoordinate = false;

        private double _mapZoom = 1.0;
        private double _mapPanX = 0.0;
        private double _mapPanY = 0.0;
  
        private const double MinMapZoom = 1.0;
        private const double MaxMapZoom = 8.0;

        private double? _homeImageX;
        private double? _homeImageY;

        private int _currentZoneIndex = -1;

        private enum GameObjectMode
        {
            Zone,
            Pickups
        }

        private GameObjectMode _gameObjectMode =
            GameObjectMode.Zone;

        private int _currentPickupIndex = -1;

        private bool _showAllZones = true;

        private readonly List<Grid> _zoneMarkers = new();
        private readonly List<Grid> _pickupMarkers = new();

#if ANDROID
        private Android.Views.View? _nativeRootView;
        private MapTouchListener? _mapTouchListener;
#endif


 
 


        private GamePlan _gamePlan = new();
        private readonly SelectedGamePlanService _selectedGamePlan;
        private bool _hasLoadedSelectedProject;

        // The containing directory is the project. Its directory name is the
        // project name; it is also persisted in gameplan.json.
        private string? _projectDirectory;


        // Snapshot of the last successfully loaded or saved game plan.
        private string? _savedStateSnapshot;

        private readonly GameObjectCoordinateService  _gameObjectCoordinateService;

        protected override bool OnBackButtonPressed()
        {
            _ = HandleBackNavigationAsync();
            return true;
        }

        private async Task HandleBackNavigationAsync()
        {
            if (_savedStateSnapshot is not null &&
                !string.Equals(
                    _savedStateSnapshot,
                    CaptureCurrentStateSnapshot(),
                    StringComparison.Ordinal))
            {
                var discardChanges = await DisplayAlert(
                    "Unsaved Changes",
                    "The game plan has changed since it was last saved. Are you sure you want to leave without saving?",
                    "Leave",
                    "Stay");

                if (!discardChanges)
                    return;
            }

            // The selected plan is shared application state. Reload it when
            // discarding editor changes so other screens never see unsaved data.
            if (_projectDirectory is not null)
            {
                try
                {
                    await _selectedGamePlan.SelectProjectAsync(_projectDirectory);
                }
                catch (Exception ex)
                {
                    await DisplayAlert(
                        "Unable to Discard Changes",
                        $"The saved game plan could not be reloaded. The editor will remain open.\n\n{ex.Message}",
                        "OK");
                    return;
                }
            }

            await Navigation.PopAsync();
        }

        private string CaptureCurrentStateSnapshot()
        {
            // Serialise the live model each time; zone and pickup allocation
            // editors update this shared GamePlan instance directly.
            return JsonSerializer.Serialize(new
            {
                GamePlan = _gamePlan,
                GameDelayTime = GameDelayTimeEntry.Text
            });
        }

        public GamePlanEditorPage(SelectedGamePlanService selectedGamePlan)
        {
            InitializeComponent();
            _selectedGamePlan = selectedGamePlan;

            // Keep Shell's standard navigation-bar back arrow and appearance,
            // but route its action through the unsaved-changes guard.
            Shell.SetBackButtonBehavior(this, new BackButtonBehavior
            {
                Command = new Command(async () =>
                    await HandleBackNavigationAsync())
            });
            GameObjectTypePicker.SelectedIndexChanged +=
                GameObjectTypePicker_SelectedIndexChanged;
            _locationService = new LocationService();
            _gameObjectCoordinateService =  new GameObjectCoordinateService();
            GameObjectTypePicker.SelectedIndex = 0;
            UpdateGameObjectControls();

#if ANDROID
            Loaded += MainPage_Loaded;
#endif
            HomeMarker.SizeChanged += HomeMarker_SizeChanged;
            MapContainer.SizeChanged += MapContainer_SizeChanged;
        }

        private void MapContainer_SizeChanged(object? sender, EventArgs e)
        {
            if (!_hasLoadedSelectedProject)
                return;

            UpdateHomeMarker();
            RenderGameObjects();
        }

        private async void MapInfoLabel_Tapped(
            object? sender,
            TappedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(MapInfoLabel.Text))
                return;

            await Clipboard.Default.SetTextAsync(
                MapInfoLabel.Text);
        }

#if ANDROID
        private void MainPage_Loaded(
            object? sender,
            EventArgs e)
        {
            // Install the listener on the page root, not on MapContainer or
            // MapTouchView.  Android can otherwise route the native touch view
            // outside its MAUI Grid column on this layout.
            if (Handler?.PlatformView
                is not Android.Views.View nativeView)
                return;

            _nativeRootView = nativeView;

            _mapTouchListener =
                new MapTouchListener(this);

            nativeView.SetOnTouchListener(
                _mapTouchListener);
        }
#endif

        protected override async void OnAppearing()
        {
            base.OnAppearing();

            if (_hasLoadedSelectedProject)
                return;

            await LoadSelectedProjectAsync();
        }

        private async Task LoadSelectedProjectAsync()
        {
            PickupAllocationButton.IsEnabled = false;
            SetHomeButton.IsEnabled = false;

            try
            {
                if (!_selectedGamePlan.IsSelected ||
                    _selectedGamePlan.ProjectDirectory is null ||
                    _selectedGamePlan.GamePlan is null ||
                    _selectedGamePlan.MapDefinition is null ||
                    _selectedGamePlan.MapBitmapPath is null)
                {
                    throw new InvalidOperationException(
                        "No game plan has been selected. Return to Game Plan Selection first.");
                }

                _projectDirectory = _selectedGamePlan.ProjectDirectory;
                _gamePlan = _selectedGamePlan.GamePlan;
                _mapDefinition = _selectedGamePlan.MapDefinition;
                _coordinateService = new MapCoordinateService(_mapDefinition);

                // Load the bitmap from the authoritative selected project.
                await using var input = File.OpenRead(_selectedGamePlan.MapBitmapPath);
                using var memory = new MemoryStream();
                await input.CopyToAsync(memory);
                var bytes = memory.ToArray();

                MapImage.Source = ImageSource.FromStream(
                    () => new MemoryStream(bytes));

                ResetMapView();

                SetHomeButton.IsEnabled = true;
                PickupAllocationButton.IsEnabled = true;
                GameDelayTimeEntry.Text = _gamePlan.GameDelayTime.ToString();

                MapInfoLabel.Text =
                    $"PROJECT: {_selectedGamePlan.ProjectName}\n" +
                    "MAP LOADED\n" +
                    "JSON LOADED\n" +
                    $"{_mapDefinition.Image.Width} × {_mapDefinition.Image.Height}\n" +
                    $"{_mapDefinition.MetresPerPixel:F4} m/pixel\n" +
                    $"Rotation: {_mapDefinition.RotationDegrees:F0}°\n" +
                    $"Centre:\n" +
                    $"{_mapDefinition.Playfield.Centre.Latitude:F6}\n" +
                    $"{_mapDefinition.Playfield.Centre.Longitude:F6}\n" +
                    $"PLAN VERSION: {_gamePlan.Version}";

                UpdateHomeMarker();
                _currentZoneIndex = _gamePlan.Zones.Count > 0 ? 0 : -1;
                _currentPickupIndex = -1;
                UpdateGameObjectControls();
                RenderGameObjects();

                _savedStateSnapshot = CaptureCurrentStateSnapshot();
                _hasLoadedSelectedProject = true;
            }
            catch (Exception ex)
            {
                MapInfoLabel.Text = "ERROR\n" + ex.Message;
                await DisplayAlert("Game Plan",
                    ex.Message, "OK");
                await Navigation.PopAsync();
            }
        }

        private async void NewGamePlan_Clicked(
            object? sender,
            EventArgs e)
        {
            if (!_selectedGamePlan.IsSelected)
                return;

            var confirmed = await DisplayAlert(
                "New Game Plan",
                "This will clear all zones, pickups, home position and allocation settings in the selected gameplan.json. This cannot be undone. Continue?",
                "New",
                "Cancel");

            if (!confirmed)
                return;

            try
            {
                await _selectedGamePlan.CreateNewGamePlanAsync();

                _gamePlan = _selectedGamePlan.GamePlan!;
                _currentZoneIndex = -1;
                _currentPickupIndex = -1;
                GameDelayTimeEntry.Text = _gamePlan.GameDelayTime.ToString();

                UpdateHomeMarker();
                UpdateGameObjectControls();
                RenderGameObjects();
                _savedStateSnapshot = CaptureCurrentStateSnapshot();

                MapInfoLabel.Text =
                    $"PROJECT: {_selectedGamePlan.ProjectName}\\n" +
                    $"NEW GAME PLAN\\nVERSION: {_gamePlan.Version}";
            }
            catch (Exception ex)
            {
                await DisplayAlert(
                    "New Game Plan",
                    $"The game plan could not be cleared.\\n\\n{ex.Message}",
                    "OK");
            }
        }

        private async void SaveGamePlan_Clicked(
            object? sender,
            EventArgs e)
        {
            try
            {
                if (!_selectedGamePlan.IsSelected)
                {
                    await DisplayAlert(
                        "Save Game Plan",
                        "Select a game plan before saving.",
                        "OK");
                    return;
                }

                if (!int.TryParse(
                        GameDelayTimeEntry.Text,
                        out var gameDelayTime) ||
                    gameDelayTime < 0)
                {
                    await DisplayAlert(
                        "Save Game Plan",
                        "Start Delay must be a whole number of seconds (0 or greater).",
                        "OK");
                    return;
                }

                _gamePlan.GameDelayTime = gameDelayTime;
                _gamePlan.ProjectName = _selectedGamePlan.ProjectName ?? string.Empty;

                await _selectedGamePlan.SaveGamePlanAsync();

                _savedStateSnapshot = CaptureCurrentStateSnapshot();
                PickupAllocationButton.IsEnabled = true;
                MapInfoLabel.Text =
                    $"PROJECT: {_selectedGamePlan.ProjectName}\n" +
                    $"GAME PLAN SAVED\nVERSION: {_gamePlan.Version}";
            }
            catch (Exception ex)
            {
                await DisplayAlert(
                    "Save Game Plan",
                    ex.Message,
                    "OK");
            }
        }

        private void UpdateMapTransform()
        {
            MapImage.Scale = _mapZoom;
            MapImage.TranslationX = _mapPanX;
            MapImage.TranslationY = _mapPanY;

            MapVisualLayer.Scale = _mapZoom;
            MapVisualLayer.TranslationX = _mapPanX;
            MapVisualLayer.TranslationY = _mapPanY;
        }

        private void ResetMapView()
        {
            _mapZoom = 1.0;
            _mapPanX = 0.0;
            _mapPanY = 0.0;

            UpdateMapTransform();
        }

        private void PositionHomeMarker(
     double imageX,
     double imageY)
        {
            if (_mapDefinition is null)
                return;

            var imageWidth =
                _mapDefinition.Image.Width;

            var imageHeight =
                _mapDefinition.Image.Height;

            var areaWidth =
                MapContainer.Width;

            var areaHeight =
                MapContainer.Height;

            if (areaWidth <= 0 ||
                areaHeight <= 0)
                return;

            var scale = Math.Min(
                areaWidth / imageWidth,
                areaHeight / imageHeight);

            var displayedWidth =
                imageWidth * scale;

            var displayedHeight =
                imageHeight * scale;

            var imageLeft =
                (areaWidth - displayedWidth) / 2.0;

            var imageTop =
                (areaHeight - displayedHeight) / 2.0;

            HomeMarker.TranslationX =
                imageLeft +
                imageX * scale -
                HomeMarker.Width / 2.0;

            HomeMarker.TranslationY =
                imageTop +
                imageY * scale -
                HomeMarker.Height / 2.0;
        }

        private async void EditObject_Clicked(
       object? sender,
       EventArgs e)
        {
            if (_gameObjectMode == GameObjectMode.Zone)
            {
                if (_currentZoneIndex < 0 ||
                    _currentZoneIndex >= _gamePlan.Zones.Count)
                {
                    return;
                }

                var zone =
                    _gamePlan.Zones[_currentZoneIndex];

                var zoneEditPage =
                    new ZoneEditPage(
                        zone,
                        _currentZoneIndex + 1);

                await Navigation.PushModalAsync(zoneEditPage);

                await zoneEditPage.Completion;

                RenderGameObjects();
                UpdateZoneInfo();

                return;
            }

            // Pickup mode
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var currentZone =
                _gamePlan.Zones[_currentZoneIndex];

            if (_currentPickupIndex < 0 ||
                _currentPickupIndex >= currentZone.Pickups.Count)
            {
                return;
            }

            var pickup =
                currentZone.Pickups[_currentPickupIndex];

            var editPage =
                new PickupEditPage(pickup);

            await Navigation.PushModalAsync(editPage);

            await editPage.Completion;

            RenderPickups();
            UpdatePickupInfo();
            UpdateGameObjectControls();

        }

        private void HandleMapTap(Point point)
        {
            if (_mapDefinition is null ||
                _coordinateService is null)
                return;

            var imageWidth =
                _mapDefinition.Image.Width;

            var imageHeight =
                _mapDefinition.Image.Height;

            var areaWidth =
                MapContainer.Width;

            var areaHeight =
                MapContainer.Height;

            if (areaWidth <= 0 ||
                areaHeight <= 0)
                return;

            // Base AspectFit rectangle at zoom = 1.
            var baseScale = Math.Min(
                areaWidth / imageWidth,
                areaHeight / imageHeight);

            var baseWidth =
                imageWidth * baseScale;

            var baseHeight =
                imageHeight * baseScale;

            var baseLeft =
                (areaWidth - baseWidth) / 2.0;

            var baseTop =
                (areaHeight - baseHeight) / 2.0;

            // Undo the pan.
            var unpannedX =
                point.X - _mapPanX;

            var unpannedY =
                point.Y - _mapPanY;

            // MapImage.Scale is centred on the element centre.
            var centreX =
                areaWidth / 2.0;

            var centreY =
                areaHeight / 2.0;

            var imageDisplayX =
                centreX +
                (unpannedX - centreX) / _mapZoom;

            var imageDisplayY =
                centreY +
                (unpannedY - centreY) / _mapZoom;

            var imageX =
                (imageDisplayX - baseLeft) / baseScale;

            var imageY =
                (imageDisplayY - baseTop) / baseScale;

            // Ignore taps outside the actual bitmap.
            if (imageX < 0 ||
                imageY < 0 ||
                imageX > imageWidth ||
                imageY > imageHeight)
            {
                MapInfoLabel.Text = "Outside map";
                return;
            }

            var gps =
                _coordinateService.ImageToGps(
                    imageX,
                    imageY);

            switch (_coordinateOperation)
            {
                case CoordinateOperation.SetHome:

                    _coordinateOperation =
                        CoordinateOperation.None;

                    SetHomePosition(gps);
                    return;

                case CoordinateOperation.AddZone:

                    _coordinateOperation =
                        CoordinateOperation.None;

                    AddZoneAtPosition(gps);
                    return;

                case CoordinateOperation.AddPickup:

                    _coordinateOperation =
                        CoordinateOperation.None;

                    AddPickupAtPosition(gps);
                    return;

            }

            // No pending placement operation.
            // If a zone is selected in Zone mode, a map tap moves it.
            if (_gameObjectMode == GameObjectMode.Zone &&
                _currentZoneIndex >= 0 &&
                _currentZoneIndex < _gamePlan.Zones.Count &&
                _gamePlan.Home is not null)
            {
                MoveZoneToPosition(gps);
                return;
            }

            // If a pickup is selected in Pickups mode,
            // a map tap moves it.
            if (_gameObjectMode == GameObjectMode.Pickups &&
                _currentZoneIndex >= 0 &&
                _currentZoneIndex < _gamePlan.Zones.Count &&
                _gamePlan.Home is not null)
            {
                MovePickupToPosition(gps);
            }


            MapInfoLabel.Text =
                $"MAP: {imageX:0.0}, {imageY:0.0}\n" +
                $"GPS:\n" +
                $"{gps.Latitude:F7}\n" +
                $"{gps.Longitude:F7}";
        }

        private void UpdateHomeMarker()
        {
            if (_gamePlan?.Home is null ||
                _coordinateService is null)
            {
                HomeMarker.IsVisible = false;
                _homeImageX = null;
                _homeImageY = null;
                return;
            }

            var home = _gamePlan.Home;

            var imagePoint =
                _coordinateService.GpsToImage(
                    home.Latitude,
                    home.Longitude);

            _homeImageX = imagePoint.X;
            _homeImageY = imagePoint.Y;

            HomeMarker.IsVisible = true;

            PositionHomeMarker(
                imagePoint.X,
                imagePoint.Y);
        }



#if ANDROID

        private sealed class MapTouchListener :
            Java.Lang.Object,
            Android.Views.View.IOnTouchListener
        {
            private readonly GamePlanEditorPage _page;

            private bool _pinching;
            private bool _wasPinching;
            private bool _singleTouchMoved;
            private bool _mapTouchActive;

            private double _touchStartX;
            private double _touchStartY;

            private double _lastX;
            private double _lastY;

            private double _startDistance;
            private double _startZoom;

            private double _startPanX;
            private double _startPanY;

            private const double TapMovementThreshold = 10.0;

            public MapTouchListener(GamePlanEditorPage page)
            {
                _page = page;
            }

            public bool OnTouch(
                Android.Views.View? v,
                Android.Views.MotionEvent? e)
            {
                if (v is null || e is null)
                    return false;

                var density =
                    v.Resources?.DisplayMetrics?.Density ?? 1.0f;

                // The listener is on the page root.  Only take ownership of
                // touches whose DOWN event is actually inside MapTouchView.
                // Returning false for everything else leaves the normal MAUI
                // hit-testing path alone, so buttons remain clickable.
                if (e.ActionMasked == Android.Views.MotionEventActions.Down)
                {
                    _mapTouchActive = IsInsideMap(e, density);

                    if (!_mapTouchActive)
                        return false;
                }

                if (!_mapTouchActive)
                    return false;

                switch (e.ActionMasked)
                {
                    case Android.Views.MotionEventActions.Down:
                        {
                            _pinching = false;
                            _wasPinching = false;
                            _singleTouchMoved = false;

                            _touchStartX =
                                GetMapX(e, 0, density);

                            _touchStartY =
                                GetMapY(e, 0, density);

                            _lastX = _touchStartX;
                            _lastY = _touchStartY;

                            break;
                        }

                    case Android.Views.MotionEventActions.PointerDown:
                        {
                            if (e.PointerCount >= 2)
                            {
                                _pinching = true;
                                _wasPinching = true;

                                _startDistance =
                                    GetDistance(e, density);

                                _startZoom =
                                    _page._mapZoom;

                                _startPanX =
                                    _page._mapPanX;

                                _startPanY =
                                    _page._mapPanY;
                            }

                            break;
                        }

                    case Android.Views.MotionEventActions.Move:
                        {
                            if (_pinching &&
                                e.PointerCount >= 2)
                            {
                                var distance =
                                    GetDistance(e, density);

                                if (_startDistance > 0)
                                {
                                    var rawZoomRatio =
                                        distance / _startDistance;

                                    // Dampen the zoom so it feels natural.
                                    var zoomRatio =
                                        1.0 +
                                        (rawZoomRatio - 1.0) * 0.35;

                                    var newZoom =
                                        Math.Clamp(
                                            _startZoom * zoomRatio,
                                            MinMapZoom,
                                            MaxMapZoom);

                                    _page._mapZoom =
                                        newZoom;

                                    _page.UpdateMapTransform();
                                }
                            }
                            else if (!_pinching &&
                                     e.PointerCount == 1)
                            {
                                var x =
                                    GetMapX(e, 0, density);

                                var y =
                                    GetMapY(e, 0, density);

                                var totalDx =
                                    x - _touchStartX;

                                var totalDy =
                                    y - _touchStartY;

                                if (Math.Sqrt(
                                        totalDx * totalDx +
                                        totalDy * totalDy)
                                    > TapMovementThreshold)
                                {
                                    _singleTouchMoved = true;
                                }

                                _page._mapPanX +=
                                    x - _lastX;

                                _page._mapPanY +=
                                    y - _lastY;

                                _lastX = x;
                                _lastY = y;

                                _page.UpdateMapTransform();
                            }

                            break;
                        }

                    case Android.Views.MotionEventActions.PointerUp:
                        {
                            _pinching = false;
                            break;
                        }

                    case Android.Views.MotionEventActions.Up:
                        {
                            var x =
                                GetMapX(e, 0, density);

                            var y =
                                GetMapY(e, 0, density);

                            // A tap is a single-finger touch that
                            // hasn't moved significantly.
                            if (!_wasPinching &&
                                !_singleTouchMoved)
                            {
                                _page.HandleMapTap(
                                    new Point(x, y));
                            }

                            _pinching = false;
                            _wasPinching = false;
                            _singleTouchMoved = false;
                            _mapTouchActive = false;

                            break;
                        }

                    case Android.Views.MotionEventActions.Cancel:
                        {
                            _pinching = false;
                            _wasPinching = false;
                            _singleTouchMoved = false;
                            _mapTouchActive = false;

                            break;
                        }
                }

                // We own the complete touch sequence.
                return true;
            }

            private bool IsInsideMap(
                Android.Views.MotionEvent e,
                double density)
            {
                if (_page.Handler?.PlatformView
                    is not Android.Views.View rootView)
                {
                    return false;
                }

                var location = new int[2];
                rootView.GetLocationOnScreen(location);

                // Convert the Android raw screen position into
                // MAUI device-independent coordinates relative
                // to the page root.
                var x =
                    e.RawX / density -
                    location[0] / density;

                var y =
                    e.RawY / density -
                    location[1] / density;

                var left =
                    _page.MapContainer.X;

                var top =
                    _page.MapContainer.Y;

                var right =
                    left + _page.MapContainer.Width;

                var bottom =
                    top + _page.MapContainer.Height;

                return x >= left &&
                       x < right &&
                       y >= top &&
                       y < bottom;
            }

            private double GetMapX(
                Android.Views.MotionEvent e,
                int pointer,
                double density)
            {
                if (_page.MapTouchView.Handler?.PlatformView
                    is not Android.Views.View mapView)
                    return 0;

                var location = new int[2];
                mapView.GetLocationOnScreen(location);

                return e.RawX / density -
                       location[0] / density;
            }

            private double GetMapY(
                Android.Views.MotionEvent e,
                int pointer,
                double density)
            {
                if (_page.MapTouchView.Handler?.PlatformView
                    is not Android.Views.View mapView)
                    return 0;

                var location = new int[2];
                mapView.GetLocationOnScreen(location);

                return e.RawY / density -
                       location[1] / density;
            }

            private static double GetDistance(
                Android.Views.MotionEvent e,
                double density)
            {
                var x1 = e.GetX(0) / density;
                var y1 = e.GetY(0) / density;

                var x2 = e.GetX(1) / density;
                var y2 = e.GetY(1) / density;

                var dx = x2 - x1;
                var dy = y2 - y1;

                return Math.Sqrt(
                    dx * dx +
                    dy * dy);
            }
        }

#endif

        private void SetHomePosition(
       MapCoordinateService.GeoPoint position)
        {
            if (_coordinateService is null)
                return;

            var oldHome =
                _gamePlan.Home;

            var zoneGpsPositions =
                new List<MapCoordinateService.GeoPoint>();

            var pickupGpsPositions =
                new List<List<MapCoordinateService.GeoPoint>>();
            if (oldHome is not null)
            {
                foreach (var zone in _gamePlan.Zones)
                {
                    var gps =
                        _gameObjectCoordinateService.LocalToGps(
                            oldHome,
                            zone.EastMetres,
                            zone.NorthMetres);

                    zoneGpsPositions.Add(gps);

                    var zonePickupPositions =
                        new List<MapCoordinateService.GeoPoint>();

                    foreach (var pickup in zone.Pickups)
                    {
                        var pickupGps =
                            _gameObjectCoordinateService.LocalToGps(
                                oldHome,
                                pickup.EastMetres,
                                pickup.NorthMetres);

                        zonePickupPositions.Add(pickupGps);
                    }

                    pickupGpsPositions.Add(zonePickupPositions);
                }
            }

            // Set the new Home.
            var newHome =
                new HomePosition
                {
                    Latitude = position.Latitude,
                    Longitude = position.Longitude
                };

            _gamePlan.Home = newHome;

            // Recalculate every existing zone relative
            // to the new Home.
            if (oldHome is not null)
            {
                for (var i = 0;
                     i < _gamePlan.Zones.Count;
                     i++)
                {
                    var zone =
                        _gamePlan.Zones[i];

                    var pickupPositions =
                        pickupGpsPositions[i];

                    for (var j = 0;
                         j < zone.Pickups.Count;
                         j++)
                    {
                        var gps =
                            pickupPositions[j];

                        var local =
                            _gameObjectCoordinateService.GpsToLocal(
                                newHome,
                                gps.Latitude,
                                gps.Longitude);

                        zone.Pickups[j].EastMetres =
                            local.EastMetres;

                        zone.Pickups[j].NorthMetres =
                            local.NorthMetres;
                    }
                }
            }

            var imagePoint =
                _coordinateService.GpsToImage(
                    position.Latitude,
                    position.Longitude);

            _homeImageX =
                imagePoint.X;

            _homeImageY =
                imagePoint.Y;

            HomeMarker.IsVisible = true;

            PositionHomeMarker(
                imagePoint.X,
                imagePoint.Y);

            UpdateGameObjectControls();
            RenderGameObjects();

            MapInfoLabel.Text =
                $"HOME SET\n" +
                $"{position.Latitude:F7}\n" +
                $"{position.Longitude:F7}";
        }

        private async void SetHome_Clicked(
            object? sender,
            EventArgs e)
        {
            var coordinate =
             await AcquireCoordinateAsync(
                 CoordinateOperation.SetHome);

            if (coordinate is not null)
            {
                SetHomePosition(coordinate.Value);
            }
        }

        private void HomeMarker_SizeChanged(
        object? sender,
        EventArgs e)
        {
            if (!_homeImageX.HasValue ||
                !_homeImageY.HasValue)
                return;

            if (HomeMarker.Width <= 0 ||
                HomeMarker.Height <= 0)
                return;

            PositionHomeMarker(
                _homeImageX.Value,
                _homeImageY.Value);
        }

        private void UpdateCoordinateInputUi()
        {
            CoordinateModeButton.Text =
                _coordinateInputMode == CoordinateInputMode.Gps
                    ? "Position: GPS"
                    : "Position: TAP";
        }

        private void SetPlacementControlsEnabled(
            bool enabled)
        {
            CoordinateModeButton.IsEnabled = enabled;

            if (enabled)
            {
                UpdateGameObjectControls();
            }
            else
            {
                AddObjectButton.IsEnabled = false;
                DeleteObjectButton.IsEnabled = false;
                EditObjectButton.IsEnabled = false;
                PreviousZoneButton.IsEnabled = false;
                NextZoneButton.IsEnabled = false;
                ShowAllObjectsButton.IsEnabled = false;
            }
        }

        private async Task<MapCoordinateService.GeoPoint?>
     AcquireCoordinateAsync(
         CoordinateOperation operation)
        {
            if (_acquiringCoordinate)
                return null;

            _coordinateOperation = operation;

            // TAP mode.
            if (_coordinateInputMode ==
                CoordinateInputMode.Tap)
            {
                MapInfoLabel.Text =
                    operation switch
                    {
                        CoordinateOperation.SetHome =>
                            "SET HOME\nTap the map",

                        CoordinateOperation.AddZone =>
                            "ADD ZONE\nTap the map",

                        CoordinateOperation.MoveZone =>
                            "MOVE ZONE\nTap the map",

                        CoordinateOperation.AddPickup =>
                            "ADD PICKUP\nTap the map",

                        _ =>
                            "Tap the map"
                    };

                return null;
            }

            // GPS mode.
            _acquiringCoordinate = true;

            try
            {
                MapInfoLabel.Text =
                    "GPS MODE\nGetting GPS position...";

                SetPlacementControlsEnabled(false);

                var result =
                    await _locationService
                        .GetCurrentPositionAsync();

                if (result.Status !=
                    LocationService.LocationStatus.Available)
                {
                    SetGpsUnavailable(
                        "GPS unavailable — TAP mode");

                    return null;
                }

                _coordinateOperation =
                    CoordinateOperation.None;

                MapInfoLabel.Text =
                    $"GPS POSITION\n" +
                    $"{result.Position!.Value.Latitude:F7}\n" +
                    $"{result.Position!.Value.Longitude:F7}";

                return result.Position;
            }
            finally
            {
                _acquiringCoordinate = false;

                SetPlacementControlsEnabled(true);
            }
        }



        private void SetCoordinateInputMode(
            CoordinateInputMode mode)
        {
            _coordinateInputMode = mode;

            _coordinateOperation =
                CoordinateOperation.None;

            UpdateCoordinateInputUi();
        }

        private async void CoordinateMode_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_coordinateInputMode ==
                CoordinateInputMode.Tap)
            {
                MapInfoLabel.Text =
                     $"Finding GPS Lock\n";

                SetPlacementControlsEnabled(false);
                var result =
                    await _locationService
                        .GetCurrentPositionAsync();
                SetPlacementControlsEnabled(true);
                if (result.Status !=
                    LocationService.LocationStatus.Available)
                {
                    SetGpsUnavailable(
                        "GPS unavailable — TAP mode");

                    return;
                }

                MapInfoLabel.Text =
                    $"Got GPS Lock\n";
                SetCoordinateInputMode(
                    CoordinateInputMode.Gps);
            }
            else
            {
                SetCoordinateInputMode(
                    CoordinateInputMode.Tap);
            }
        }

        private void SetGpsUnavailable(
            string message)
        {
            _coordinateInputMode =
                CoordinateInputMode.Tap;

            _coordinateOperation =
                CoordinateOperation.None;

            UpdateCoordinateInputUi();

            MapInfoLabel.Text =
                message;
        }

        private void UpdateGameObjectControls()
        {
            var homeAvailable =
                _gamePlan.Home is not null;

            if (!homeAvailable)
            {
                // A valid map is enough to begin creating zones. Setting Home
                // is still required before a zone can be placed, because its
                // coordinates are stored relative to Home.
                AddObjectButton.IsEnabled =
                    _mapDefinition is not null &&
                    _gameObjectMode == GameObjectMode.Zone;
                PreviousZoneButton.IsEnabled = false;
                NextZoneButton.IsEnabled = false;
                DeleteObjectButton.IsEnabled = false;
                EditObjectButton.IsEnabled = false;
                ShowAllObjectsButton.IsEnabled = false;

                CurrentObjectLabel.Text = "0 / 0";
                return;
            }

            if (_gameObjectMode == GameObjectMode.Zone)
            {
                var objectsAvailable =
                    _gamePlan.Zones.Count > 0;

                AddObjectButton.IsEnabled = true;
                PreviousZoneButton.IsEnabled = objectsAvailable;
                NextZoneButton.IsEnabled = objectsAvailable;
                DeleteObjectButton.IsEnabled = objectsAvailable;
                EditObjectButton.IsEnabled = objectsAvailable;
                ShowAllObjectsButton.IsEnabled = objectsAvailable;

                CurrentObjectLabel.Text =
                    objectsAvailable
                        ? $"{_currentZoneIndex + 1} / {_gamePlan.Zones.Count}"
                        : "0 / 0";

                return;
            }

            // Pickups mode.
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                AddObjectButton.IsEnabled = false;
                PreviousZoneButton.IsEnabled = false;
                NextZoneButton.IsEnabled = false;
                DeleteObjectButton.IsEnabled = false;
                EditObjectButton.IsEnabled = false;
                ShowAllObjectsButton.IsEnabled = false;

                CurrentObjectLabel.Text = "0 / 0";
                return;
            }

            var pickups =
                _gamePlan.Zones[_currentZoneIndex].Pickups;

            var pickupsAvailable =
                pickups.Count > 0;

            AddObjectButton.IsEnabled = true;
            PreviousZoneButton.IsEnabled = pickupsAvailable;
            NextZoneButton.IsEnabled = pickupsAvailable;
            DeleteObjectButton.IsEnabled = pickupsAvailable;
            EditObjectButton.IsEnabled = pickupsAvailable;
            ShowAllObjectsButton.IsEnabled = false;
            
            // "Show All" doesn't have a useful meaning for pickups yet.
            ShowAllObjectsButton.IsEnabled = false;

            CurrentObjectLabel.Text =
                pickupsAvailable
                    ? $"{_currentPickupIndex + 1} / {pickups.Count}"
                    : "0 / 0";
        }

        private void PreviousZone_Clicked(
      object? sender,
      EventArgs e)
        {

            if (_gamePlan.Zones.Count == 0)
                return;


            if (_gameObjectMode == GameObjectMode.Pickups)
            {
                if (_currentZoneIndex < 0 ||
                    _currentZoneIndex >= _gamePlan.Zones.Count)
                {
                    return;
                }

                var pickups =
                    _gamePlan.Zones[_currentZoneIndex].Pickups;

                if (pickups.Count == 0)
                    return;

                if (_currentPickupIndex < 0)
                {
                    _currentPickupIndex =
                        pickups.Count - 1;
                }
                else if (_currentPickupIndex > 0)
                {
                    _currentPickupIndex--;
                }

                RenderPickups();
                UpdatePickupInfo();
                UpdateGameObjectControls();

                return;
            }
            else 
            {
                if (_currentZoneIndex > 0)
                    _currentZoneIndex--;

                _currentPickupIndex = -1;

                UpdateZoneInfo();
                UpdateGameObjectControls();
                RenderGameObjects();
            }

        }

        private void NextZone_Clicked(
           object? sender,
           EventArgs e)
        {
            if (_gamePlan.Zones.Count == 0)
                return;

            if (_gameObjectMode == GameObjectMode.Pickups)
            {
                if (_currentZoneIndex < 0 ||
                    _currentZoneIndex >= _gamePlan.Zones.Count)
                {
                    return;
                }

                var pickups =
                    _gamePlan.Zones[_currentZoneIndex].Pickups;

                if (pickups.Count == 0)
                    return;

                if (_currentPickupIndex < 0)
                {
                    _currentPickupIndex = 0;
                }
                else if (_currentPickupIndex <
                         pickups.Count - 1)
                {
                    _currentPickupIndex++;
                }

                RenderPickups();
                UpdatePickupInfo();
                UpdateGameObjectControls();

                return;
            }
            else 
            {

                if (_currentZoneIndex <
                    _gamePlan.Zones.Count - 1)
                {
                    _currentZoneIndex++;
                }

                _currentPickupIndex = -1;

                UpdateZoneInfo();
                UpdateGameObjectControls();
                RenderGameObjects();
            }
        }

        private async void MoveZone_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count ||
                _gamePlan.Home is null)
            {
                return;
            }

            var coordinate =
                await AcquireCoordinateAsync(
                    CoordinateOperation.MoveZone);

            if (coordinate is null)
                return;

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    coordinate.Value.Latitude,
                    coordinate.Value.Longitude);

            zone.EastMetres =
                local.EastMetres;

            zone.NorthMetres =
                local.NorthMetres;

            RenderGameObjects();

            UpdateZoneInfo();
        }

        private void ShowAllZones_Clicked(
    object? sender,
    EventArgs e)
        {
            _showAllZones =
                !_showAllZones;

            UpdateGameObjectControls();

            RenderGameObjects();
        }

        private void RenderPickups()
        {
            ClearPickupMarkers();

            if (_gameObjectMode != GameObjectMode.Pickups)
                return;

            if (_mapDefinition is null ||
                _coordinateService is null ||
                _gamePlan.Home is null)
            {
                return;
            }

            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            for (var i = 0;
                 i < zone.Pickups.Count;
                 i++)
            {
                var pickup =
                    zone.Pickups[i];

                var gps =
                    _gameObjectCoordinateService.LocalToGps(
                        _gamePlan.Home,
                        pickup.EastMetres,
                        pickup.NorthMetres);

                var imagePoint =
                    _coordinateService.GpsToImage(
                        gps.Latitude,
                        gps.Longitude);

                var marker =
                    CreatePickupMarker(
                        pickup,
                        i);

                PositionPickupMarker(
                    marker,
                    imagePoint.X,
                    imagePoint.Y);

                MapVisualLayer.Children.Add(marker);
                _pickupMarkers.Add(marker);
            }
        }

        private void RenderZones()
        {
            if (_mapDefinition is null ||
                _coordinateService is null ||
                _gamePlan.Home is null)
            {
                ClearZoneMarkers();
                return;
            }

            ClearZoneMarkers();

            for (var i = 0;
                 i < _gamePlan.Zones.Count;
                 i++)
            {
                if (!_showAllZones &&
                    i != _currentZoneIndex)
                {
                    continue;
                }

                var zone =
                    _gamePlan.Zones[i];

                var gps =
                    _gameObjectCoordinateService.LocalToGps(
                        _gamePlan.Home,
                        zone.EastMetres,
                        zone.NorthMetres);

                var imagePoint =
                    _coordinateService.GpsToImage(
                        gps.Latitude,
                        gps.Longitude);

                var marker =
                    CreateZoneMarker(
                        i,
                        zone.RadiusMetres);

                PositionZoneMarker(
                    marker,
                    imagePoint.X,
                    imagePoint.Y,
                    zone.RadiusMetres);

                MapVisualLayer.Children.Add(marker);

                _zoneMarkers.Add(marker);
            }
        }

        private void RenderGameObjects()
        {
            RenderZones();

            if (_gameObjectMode == GameObjectMode.Pickups)
            {
                RenderPickups();
            }
            else
            {
                ClearPickupMarkers();
            }
        }

        private Grid CreateZoneMarker(
    int zoneIndex,
    double radiusMetres)
        {
            var marker =
                new Grid
                {
                    WidthRequest = 20,
                    HeightRequest = 20,
                    HorizontalOptions = LayoutOptions.Start,
                    VerticalOptions = LayoutOptions.Start,
                    InputTransparent = true
                };

            var circle =
                new Border
                {
                    Stroke =
                        zoneIndex == _currentZoneIndex
                            ? Colors.Green
                            : Colors.Yellow,

                    StrokeThickness =
                        zoneIndex == _currentZoneIndex
                            ? 3
                            : 2,

                    BackgroundColor =
                        Colors.Transparent,

                    StrokeShape =
                        new RoundRectangle
                        {
                            CornerRadius = 999
                        },

                    InputTransparent = true
                };

            var number =
                new Label
                {
                    Text = (zoneIndex + 1).ToString(),
                    FontSize = 14,
                    FontAttributes = FontAttributes.Bold,
                    TextColor = Colors.White,
                    HorizontalTextAlignment =
                        TextAlignment.Center,
                    VerticalTextAlignment =
                        TextAlignment.Center,
                    HorizontalOptions =
                        LayoutOptions.Center,
                    VerticalOptions =
                        LayoutOptions.Center,
                    InputTransparent = true
                };

            var centre =
                new Label
                {
                    Text = "+",
                    FontSize = 16,
                    FontAttributes = FontAttributes.Bold,
                    TextColor = Colors.White,
                    HorizontalTextAlignment =
                        TextAlignment.Center,
                    VerticalTextAlignment =
                        TextAlignment.Center,
                    HorizontalOptions =
                        LayoutOptions.Center,
                    VerticalOptions =
                        LayoutOptions.Center,
                    InputTransparent = true
                };

            marker.Children.Add(circle);
            marker.Children.Add(number);
            marker.Children.Add(centre);

            return marker;
        }

        private void PositionZoneMarker(
    Grid marker,
    double imageX,
    double imageY,
    double radiusMetres)
        {
            if (_mapDefinition is null)
                return;

            var imageWidth =
                _mapDefinition.Image.Width;

            var imageHeight =
                _mapDefinition.Image.Height;

            var areaWidth =
                MapContainer.Width;

            var areaHeight =
                MapContainer.Height;

            if (areaWidth <= 0 ||
                areaHeight <= 0)
                return;

            var scale =
                Math.Min(
                    areaWidth / imageWidth,
                    areaHeight / imageHeight);

            var displayedWidth =
                imageWidth * scale;

            var displayedHeight =
                imageHeight * scale;

            var imageLeft =
                (areaWidth - displayedWidth) / 2.0;

            var imageTop =
                (areaHeight - displayedHeight) / 2.0;

            var radiusPixels =
                radiusMetres /
                _mapDefinition.MetresPerPixel;

            var diameter =
                radiusPixels * 2.0 * scale;

            // Don't let a tiny zone become invisible.
            diameter =
                Math.Max(diameter, 20.0);

            marker.WidthRequest =
                diameter;

            marker.HeightRequest =
                diameter;

            if (marker.Children.Count > 0 &&
            marker.Children[0] is Border circle)
            {
                circle.WidthRequest =
                    diameter;

                circle.HeightRequest =
                    diameter;
            }

            marker.TranslationX =
                imageLeft +
                imageX * scale -
                diameter / 2.0;

            marker.TranslationY =
                imageTop +
                imageY * scale -
                diameter / 2.0;
        }

        private void ClearZoneMarkers()
        {
            foreach (var marker in _zoneMarkers)
            {
                MapVisualLayer.Children.Remove(marker);
            }

            _zoneMarkers.Clear();
        }

        private void UpdatePickupInfo()
        {
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var pickups =
                _gamePlan.Zones[_currentZoneIndex].Pickups;

            if (_currentPickupIndex < 0 ||
                _currentPickupIndex >= pickups.Count)
            {
                MapInfoLabel.Text =
                    $"ZONE {_currentZoneIndex + 1}\n" +
                    "No pickups";
                return;
            }

            var pickup =
                pickups[_currentPickupIndex];

            MapInfoLabel.Text =
                $"PICKUP {_currentPickupIndex + 1}\n" +
                $"Type: {pickup.Type}\n" +
                $"E: {pickup.EastMetres:F1} m\n" +
                $"N: {pickup.NorthMetres:F1} m\n" +
                $"Spawn: {pickup.SpawnTimeSeconds} s\n" +
                $"Lifespan: {pickup.LifespanSeconds} s";
        }
        private void UpdateZoneInfo()
        {
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            MapInfoLabel.Text =
                $"ZONE {_currentZoneIndex + 1}\n" +
                $"E: {zone.EastMetres:F1} m\n" +
                $"N: {zone.NorthMetres:F1} m\n" +
                $"Radius: {zone.RadiusMetres:F1} m\n" +
                $"Start: {zone.StartTime}\n" +
                $"Lifespan: {zone.LifespanSeconds}";
        }

        private int GetNewZoneStartTime()
        {
            // Append after the latest finite zone end. A very large lifespan is
            // the timeline's permanent-zone sentinel, so it must not push a new
            // zone millions of seconds into the future.
            var finiteEnds = _gamePlan.Zones
                .Where(z => z.LifespanSeconds > 0 && z.LifespanSeconds < 1_000_000)
                .Select(z => (long)Math.Max(0, z.StartTime) + z.LifespanSeconds)
                .ToList();

            var latestStart = _gamePlan.Zones.Count > 0
                ? _gamePlan.Zones.Max(z => (long)Math.Max(0, z.StartTime))
                : 0L;
            var end = Math.Max(latestStart, finiteEnds.Count > 0 ? finiteEnds.Max() : 0L);

            return (int)Math.Clamp(end, 0L, int.MaxValue);
        }

        private void CloseGapAfterZoneDeletion(int deletedIndex)
        {
            var zones = _gamePlan.Zones;
            if (zones.Count == 0)
                return;
            if (deletedIndex == 0)
            {
                // The first remaining zone becomes Zone 0 and is anchored at zero.
                zones[0].StartTime = 0;
                return;
            }
            if (zones.Count < 2 || deletedIndex >= zones.Count)
                return;

            // Zone 0 remains anchored at zero. If removing a zone leaves a gap
            // between its neighbours, shift the later schedule left just enough
            // to close that gap. Existing overlaps and relative timings remain.
            var previous = zones[deletedIndex - 1];
            var next = zones[deletedIndex];
            var previousExpiry = (long)previous.StartTime + previous.LifespanSeconds;
            var gap = (long)next.StartTime - previousExpiry;
            if (gap <= 0)
                return;

            for (var i = deletedIndex; i < zones.Count; i++)
            {
                var shifted = (long)zones[i].StartTime - gap;
                zones[i].StartTime = (int)Math.Clamp(shifted, i == 0 ? 0L : 0L, int.MaxValue);
            }
        }

        private async void AddZone_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_mapDefinition is null ||
                _coordinateService is null)
                return;

            if (_gamePlan.Home is null)
            {
                await DisplayAlert(
                    "Add Zone",
                    "Set Home before adding a zone.",
                    "OK");
                return;
            }

            var coordinate =
                await AcquireCoordinateAsync(
                    CoordinateOperation.AddZone);

            if (coordinate is null)
                return;

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    coordinate.Value.Latitude,
                    coordinate.Value.Longitude);

            var zone =
                new GameZone
                {
                    EastMetres = local.EastMetres,
                    NorthMetres = local.NorthMetres,
                    RadiusMetres = 50.0,
                    StartTime = GetNewZoneStartTime(),
                    LifespanSeconds = 300
                };

            _gamePlan.Zones.Add(zone);

            _currentZoneIndex =
                _gamePlan.Zones.Count - 1;
            _currentPickupIndex = -1;

            UpdateGameObjectControls();

            RenderGameObjects();

            MapInfoLabel.Text =
                $"ZONE {_currentZoneIndex + 1} ADDED\n" +
                $"E: {zone.EastMetres:F1} m\n" +
                $"N: {zone.NorthMetres:F1} m\n" +
                $"Radius: {zone.RadiusMetres:F1} m";
        }

        private void AddZoneAtPosition(
    MapCoordinateService.GeoPoint position)
        {
            if (_gamePlan.Home is null)
                return;

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    position.Latitude,
                    position.Longitude);

            var zone =
                new GameZone
                {
                    EastMetres = local.EastMetres,
                    NorthMetres = local.NorthMetres,
                    RadiusMetres = 50.0,
                    StartTime = GetNewZoneStartTime(),
                    LifespanSeconds = 300
                };

            _gamePlan.Zones.Add(zone);

            _currentZoneIndex =
                _gamePlan.Zones.Count - 1;
            _currentPickupIndex = -1;

            UpdateGameObjectControls();
            RenderGameObjects();
            UpdateZoneInfo();
        }

        private void MoveZoneToPosition(
    MapCoordinateService.GeoPoint position)
        {
            if (_gamePlan.Home is null ||
                _currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
                return;

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    position.Latitude,
                    position.Longitude);

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            zone.EastMetres =
                local.EastMetres;

            zone.NorthMetres =
                local.NorthMetres;

            RenderGameObjects();
            UpdateZoneInfo();
        }

        private void AddObject_Clicked(
     object? sender,
     EventArgs e)
        {
            if (_gameObjectMode == GameObjectMode.Zone)
            {
                AddZone_Clicked(sender, e);
                return;
            }

            AddPickup_Clicked(sender, e);
        }


        private async void ShowAllObjects_Clicked(
    object? sender,
    EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex != 0)
                return;

            // Open the graphical zone timeline. The previous handler toggled
            // the map's all-zones overlay instead of opening the timeline.
            await Navigation.PushModalAsync(
                new ZoneTimelinePage(_gamePlan));
        }

        private void Hamburger_Clicked(
    object? sender,
    EventArgs e)
        {
            Shell.Current.FlyoutIsPresented = true;
        }


        private async void DeleteObject_Clicked(
            object? sender,
            EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex == 0)
            {
                // Delete Zone
                if (_currentZoneIndex < 0 ||
                    _currentZoneIndex >= _gamePlan.Zones.Count)
                {
                    return;
                }

                var zoneNumber =
                    _currentZoneIndex + 1;

                var confirmed =
                    await DisplayAlert(
                        "Delete Zone",
                        $"Delete Zone {zoneNumber}?",
                        "Delete",
                        "Cancel");

                if (!confirmed)
                    return;

                var deletedIndex = _currentZoneIndex;
                _gamePlan.Zones.RemoveAt(deletedIndex);
                CloseGapAfterZoneDeletion(deletedIndex);

                if (_gamePlan.Zones.Count == 0)
                {
                    _currentZoneIndex = -1;
                }
                else if (_currentZoneIndex >=
                         _gamePlan.Zones.Count)
                {
                    _currentZoneIndex =
                        _gamePlan.Zones.Count - 1;
                }

                _currentPickupIndex = -1;

                UpdateGameObjectControls();
                RenderGameObjects();
                UpdateZoneInfo();

                return;
            }

            // Delete Pickup
            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            if (_currentPickupIndex < 0 ||
                _currentPickupIndex >= zone.Pickups.Count)
            {
                return;
            }

            var pickup =
                zone.Pickups[_currentPickupIndex];

            var confirmedPickup =
                await DisplayAlert(
                    "Delete Pickup",
                    $"Delete Pickup {_currentPickupIndex + 1}?",
                    "Delete",
                    "Cancel");

            if (!confirmedPickup)
                return;

            zone.Pickups.RemoveAt(
                _currentPickupIndex);

            if (zone.Pickups.Count == 0)
            {
                _currentPickupIndex = -1;
            }
            else if (_currentPickupIndex >=
                     zone.Pickups.Count)
            {
                _currentPickupIndex =
                    zone.Pickups.Count - 1;
            }

            UpdateGameObjectControls();
            RenderPickups();
            UpdatePickupInfo();
        }

        private void GameObjectTypePicker_SelectedIndexChanged(
     object? sender,
     EventArgs e)
        {
            _gameObjectMode =
                GameObjectTypePicker.SelectedIndex == 1
                    ? GameObjectMode.Pickups
                    : GameObjectMode.Zone;

            if(_gameObjectMode == GameObjectMode.Pickups)
            {
                _currentPickupIndex = -1;
                if (_currentZoneIndex >= 0 &&
                  _currentZoneIndex < _gamePlan.Zones.Count)
                {
                    if (_gamePlan.Zones[_currentZoneIndex].Pickups.Count != 0)
                    {
                        _currentPickupIndex = 0;
                    }
                }
            }

            

            UpdateGameObjectControls();
            RenderGameObjects();
        }

        private async void AddPickup_Clicked(
            object? sender,
            EventArgs e)
        {
            if (_gamePlan.Home is null)
                return;

            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var coordinate =
                await AcquireCoordinateAsync(
                    CoordinateOperation.AddPickup);

            if (coordinate is null)
                return;

            AddPickupAtPosition(coordinate.Value);
        }

        private void AddPickupAtPosition(
    MapCoordinateService.GeoPoint position)
        {
            if (_gamePlan.Home is null)
                return;

            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    position.Latitude,
                    position.Longitude);

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            var pickup =
                new GamePickup
                {
                    Id = _gamePlan. AllocatePickupId(),
                    EastMetres = local.EastMetres,
                    NorthMetres = local.NorthMetres,
                    Type = PickupType.Auto,
                    SpawnTimeSeconds = 0,
                    LifespanSeconds = 300
                };

            zone.Pickups.Add(pickup);

            _currentPickupIndex =
                zone.Pickups.Count - 1;

            _coordinateOperation =
                CoordinateOperation.None;

            UpdateGameObjectControls();
            RenderPickups();
            UpdatePickupInfo();
        }

        private void MovePickupToPosition(
    MapCoordinateService.GeoPoint position)
        {
            if (_gamePlan.Home is null)
                return;

            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            if (_currentPickupIndex < 0 ||
                _currentPickupIndex >= zone.Pickups.Count)
            {
                return;
            }

            var local =
                _gameObjectCoordinateService.GpsToLocal(
                    _gamePlan.Home,
                    position.Latitude,
                    position.Longitude);

            var pickup =
                zone.Pickups[_currentPickupIndex];

            pickup.EastMetres =
                local.EastMetres;

            pickup.NorthMetres =
                local.NorthMetres;

            RenderPickups();
            UpdatePickupInfo();
        }

        private static ImageSource GetPickupImageSource(
        PickupType type)
        {
            return type switch
            {
                PickupType.Health =>
                    ImageSource.FromFile(
                        "pickup_health.png"),

                PickupType.Reload =>
                    ImageSource.FromFile(
                        "pickup_reload.png"),

                PickupType.WeaponBonus =>
                    ImageSource.FromFile(
                        "pickup_weapon_bonus.png"),

                PickupType.Auto =>
                              ImageSource.FromFile(
                                  "auto.png"),

                _ =>
                    ImageSource.FromFile(
                        "pickup_health.png")
            };
        }

        private void PositionPickupMarker(
    Grid marker,
    double imageX,
    double imageY)
        {
            if (_mapDefinition is null)
                return;

            
            var imageWidth =
                _mapDefinition.Image.Width;

            var imageHeight =
                _mapDefinition.Image.Height;

            var areaWidth =
                MapContainer.Width;

            var areaHeight =
                MapContainer.Height;

            if (areaWidth <= 0 ||
                areaHeight <= 0)
            {
                return;
            }

            var scale =
                Math.Min(
                    areaWidth / imageWidth,
                    areaHeight / imageHeight);

            var displayedWidth =
                imageWidth * scale;

            var displayedHeight =
                imageHeight * scale;

            var imageLeft =
                (areaWidth - displayedWidth) / 2.0;

            var imageTop =
                (areaHeight - displayedHeight) / 2.0;

            marker.TranslationX =
                imageLeft +
                imageX * scale -
                marker.WidthRequest / 2.0;

            marker.TranslationY =
                imageTop +
                imageY * scale -
                marker.HeightRequest / 2.0;
        }

        private void ClearPickupMarkers()
        {
            foreach (var marker in _pickupMarkers)
            {
                MapVisualLayer.Children.Remove(marker);
            }

            _pickupMarkers.Clear();
        }

        private Grid CreatePickupMarker(
         GamePickup pickup,
         int pickupIndex)
        {
            var marker =
                new Grid
                {
                    WidthRequest = 40,
                    HeightRequest = 40,
                    HorizontalOptions = LayoutOptions.Start,
                    VerticalOptions = LayoutOptions.Start,
                    InputTransparent = true
                };

            var image =
                new Image
                {
                    Source = GetPickupImageSource(
                        pickup.Type),
                    WidthRequest = 32,
                    HeightRequest = 32,
                    Aspect = Aspect.AspectFit,
                    HorizontalOptions = LayoutOptions.Center,
                    VerticalOptions = LayoutOptions.Center,
                    InputTransparent = true
                };

            if (_gameObjectMode == GameObjectMode.Pickups &&
                pickupIndex == _currentPickupIndex)
            {
                var highlight =
                    new Border
                    {
                        Stroke = Colors.Green,
                        StrokeThickness = 3,
                        BackgroundColor = Colors.Transparent,
                        StrokeShape =
                            new RoundRectangle
                            {
                                CornerRadius = 999
                            },
                        Padding = 2,
                        InputTransparent = true
                    };

                highlight.Content = image;

                marker.Children.Add(highlight);
            }
            else
            {
                marker.Children.Add(image);
            }

            return marker;
        }


    }


}