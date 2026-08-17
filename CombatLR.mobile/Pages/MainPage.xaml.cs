using CombatLR.mobile.Models;
using CombatLR.mobile.PageModels;
using CombatLR.mobile.Services;
using Microsoft.Maui.Controls.Shapes;
using System.Security.Cryptography.X509Certificates;
using System.Text.Json;


namespace CombatLR.mobile.Pages
{
    public partial class MainPage : ContentPage
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
            MoveZone
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

        private bool _showAllZones = true;

        private readonly List<Grid> _zoneMarkers = new();

#if ANDROID
        private Android.Views.View? _nativeMapView;
        private MapTouchListener? _mapTouchListener;
#endif

        private double _panStartX;
        private double _panStartY;
        private double _panStartMapX;
        private double _panStartMapY;

        private double _pinchStartZoom;
        private double _pinchStartPanX;
        private double _pinchStartPanY;
        private double _pinchLastScale = 1.0;
        private GamePlan _gamePlan = new();

        private readonly GamePlanService _gamePlanService = new();

        private readonly GameObjectCoordinateService  _gameObjectCoordinateService;

        public MainPage(MainPageModel model)
        {
            InitializeComponent();
            BindingContext = model;
            _locationService = new LocationService();
            _gameObjectCoordinateService =  new GameObjectCoordinateService();

            UpdateGameObjectControls();

#if ANDROID
            Loaded += MainPage_Loaded;
#endif
            HomeMarker.SizeChanged += HomeMarker_SizeChanged;
            _ = LoadMapAsync();


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
            if (MapContainer.Handler?.PlatformView
                is not Android.Views.View nativeView)
                return;

            _nativeMapView = nativeView;

            _mapTouchListener =
                new MapTouchListener(this);

            nativeView.SetOnTouchListener(
                _mapTouchListener);
        }
#endif

        private async Task LoadMapAsync()
        {
            try
            {
                // Load the bitmap.
                await using var input =
                    await FileSystem.OpenAppPackageFileAsync("map.bmp");

                using var memory = new MemoryStream();
                await input.CopyToAsync(memory);

                var bytes = memory.ToArray();

                MapImage.Source = ImageSource.FromStream(
                    () => new MemoryStream(bytes));

                MapInfoLabel.Text =
                    "MAP LOADED\nLoading map.json...";

                ResetMapView();

                // Load map.json.
                await using var jsonStream =
                    await FileSystem.OpenAppPackageFileAsync("map.json");

                var mapDefinition =
                    await JsonSerializer.DeserializeAsync<MapDefinition>(
                        jsonStream);

                if (mapDefinition is null)
                    throw new InvalidDataException(
                        "map.json produced no MapDefinition.");

                _mapDefinition = mapDefinition;

                _coordinateService =
                    new MapCoordinateService(_mapDefinition);

                MapInfoLabel.Text =
                    $"MAP LOADED\n" +
                    $"JSON LOADED\n" +
                    $"{_mapDefinition.Image.Width} × {_mapDefinition.Image.Height}\n" +
                    $"{_mapDefinition.MetresPerPixel:F4} m/pixel\n" +
                    $"Rotation: {_mapDefinition.RotationDegrees:F0}°\n" +
                    $"Centre:\n" +
                    $"{_mapDefinition.Playfield.Centre.Latitude:F6}\n" +
                    $"{_mapDefinition.Playfield.Centre.Longitude:F6}";
            }
            catch (Exception ex)
            {
                MapInfoLabel.Text =
                    "ERROR\n" + ex.Message;

                await DisplayAlert(
                    "Map Error",
                    ex.ToString(),
                    "OK");
            }
        }

        private async void NewGamePlan_Clicked(
        object? sender,
        EventArgs e)
        {
            _gamePlan = _gamePlanService.CreateNew();
            ClearZoneMarkers();

            _currentZoneIndex = -1;
            HomeMarker.IsVisible = false;
            UpdateGameObjectControls();
      
            MapInfoLabel.Text =
                "NEW GAME PLAN\n" +
                "No Home\n" +
                "0 Zones";
        }

        private async void SaveGamePlan_Clicked(
        object? sender,
        EventArgs e)
        {
            try
            {
                var fileName = "gameplan.json";

                var filePath =
                   System.IO.Path.Combine(
                        FileSystem.Current.AppDataDirectory,
                        fileName);

                await _gamePlanService.SaveAsync(
                    _gamePlan,
                    filePath);

                MapInfoLabel.Text =
                    $"GAME PLAN SAVED\n{fileName}";
            }
            catch (Exception ex)
            {
                await DisplayAlert(
                    "Save Game Plan",
                    ex.Message,
                    "OK");
            }
        }

        private async void LoadGamePlan_Clicked(
    object? sender,
    EventArgs e)
        {
            try
            {
                var fileName = "gameplan.json";

                var filePath =
                    System.IO.Path.Combine(
                        FileSystem.Current.AppDataDirectory,
                        fileName);

                if (!File.Exists(filePath))
                {
                    await DisplayAlert(
                        "Load Game Plan",
                        "No game plan has been saved yet.",
                        "OK");

                    return;
                }

                _gamePlan =
                    await _gamePlanService.LoadAsync(
                        filePath);

                UpdateGameObjectControls();
                UpdateHomeMarker();
                _currentZoneIndex =
                    _gamePlan.Zones.Count > 0
                    ? 0
                    : -1;

                RenderZones();
                UpdateGameObjectControls();

                MapInfoLabel.Text =
                    $"GAME PLAN LOADED\n" +
                    $"Home: {(_gamePlan.Home is null ? "None" : "Set")}\n" +
                    $"Zones: {_gamePlan.Zones.Count}";
            }
            catch (Exception ex)
            {
                await DisplayAlert(
                    "Load Game Plan",
                    $"The game plan could not be loaded.\n\n{ex.Message}",
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

                case CoordinateOperation.MoveZone:

                    _coordinateOperation =
                        CoordinateOperation.None;

                    MoveZoneToPosition(gps);
                    return;
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
            private readonly MainPage _page;

            private bool _pinching;
            private bool _wasPinching;
            private bool _singleTouchMoved;

            private double _touchStartX;
            private double _touchStartY;

            private double _lastX;
            private double _lastY;

            private double _startDistance;
            private double _startZoom;

            private double _startPanX;
            private double _startPanY;

            private const double TapMovementThreshold = 10.0;

            public MapTouchListener(MainPage page)
            {
                _page = page;
            }

            public bool OnTouch(
                Android.Views.View? v,
                Android.Views.MotionEvent? e)
            {
                if (v is null || e is null)
                    return true;

                var density =
                    v.Resources?.DisplayMetrics?.Density ?? 1.0f;

                switch (e.ActionMasked)
                {
                    case Android.Views.MotionEventActions.Down:
                        {
                            _pinching = false;
                            _wasPinching = false;
                            _singleTouchMoved = false;

                            _touchStartX =
                                e.GetX(0) / density;

                            _touchStartY =
                                e.GetY(0) / density;

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
                                    e.GetX(0) / density;

                                var y =
                                    e.GetY(0) / density;

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
                                e.GetX(0) / density;

                            var y =
                                e.GetY(0) / density;

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

                            break;
                        }

                    case Android.Views.MotionEventActions.Cancel:
                        {
                            _pinching = false;
                            _wasPinching = false;
                            _singleTouchMoved = false;

                            break;
                        }
                }

                // We own the complete touch sequence.
                return true;
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

            // If Home already exists, first convert every
            // existing game object to an absolute GPS position.
            var zoneGpsPositions =
                new List<MapCoordinateService.GeoPoint>();

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
                    var gps =
                        zoneGpsPositions[i];

                    var local =
                        _gameObjectCoordinateService.GpsToLocal(
                            newHome,
                            gps.Latitude,
                            gps.Longitude);

                    _gamePlan.Zones[i].EastMetres =
                        local.EastMetres;

                    _gamePlan.Zones[i].NorthMetres =
                        local.NorthMetres;
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
            RenderZones();

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
                MoveObjectButton.IsEnabled = false;
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

            var objectsAvailable =
                homeAvailable &&
                _gamePlan.Zones.Count > 0;

            AddObjectButton.IsEnabled =
                homeAvailable;

            PreviousZoneButton.IsEnabled =
                objectsAvailable;

            NextZoneButton.IsEnabled =
                objectsAvailable;

            MoveObjectButton.IsEnabled =
                objectsAvailable;

            EditObjectButton.IsEnabled =
                objectsAvailable;

            ShowAllObjectsButton.IsEnabled =
                objectsAvailable;

            CurrentObjectLabel.Text =
                objectsAvailable
                    ? $"{_currentZoneIndex + 1} / {_gamePlan.Zones.Count}"
                    : "0 / 0";
        }

        private void PreviousZone_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_gamePlan.Zones.Count == 0)
                return;

            if (_currentZoneIndex > 0)
                _currentZoneIndex--;

            RenderZones();

            UpdateZoneInfo();
        }

        private void NextZone_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_gamePlan.Zones.Count == 0)
                return;

            if (_currentZoneIndex <
                _gamePlan.Zones.Count - 1)
            {
                _currentZoneIndex++;
            }

            RenderZones();

            UpdateZoneInfo();
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

            RenderZones();

            UpdateZoneInfo();
        }

        private void ShowAllZones_Clicked(
    object? sender,
    EventArgs e)
        {
            _showAllZones =
                !_showAllZones;

            UpdateGameObjectControls();

            RenderZones();
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
                $"Transition: {zone.TransitionTime}";
        }

        private async void AddZone_Clicked(
    object? sender,
    EventArgs e)
        {
            if (_gamePlan.Home is null)
                return;

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
                    StartTime = TimeSpan.Zero,
                    TransitionTime = TimeSpan.Zero
                };

            _gamePlan.Zones.Add(zone);

            _currentZoneIndex =
                _gamePlan.Zones.Count - 1;

            UpdateGameObjectControls();

            RenderZones();

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
                    StartTime = TimeSpan.Zero,
                    TransitionTime = TimeSpan.Zero
                };

            _gamePlan.Zones.Add(zone);

            _currentZoneIndex =
                _gamePlan.Zones.Count - 1;

            UpdateGameObjectControls();
            RenderZones();
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

            RenderZones();
            UpdateZoneInfo();
        }

        private void AddObject_Clicked(
    object? sender,
    EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex != 0)
                return;

            AddZone_Clicked(sender, e);
        }

        private void MoveObject_Clicked(
    object? sender,
    EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex != 0)
                return;

            MoveZone_Clicked(sender, e);
        }
        private async void EditObject_Clicked(
    object? sender,
    EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex != 0)
                return;

            if (_currentZoneIndex < 0 ||
                _currentZoneIndex >= _gamePlan.Zones.Count)
            {
                return;
            }

            var zone =
                _gamePlan.Zones[_currentZoneIndex];

            await Navigation.PushModalAsync(
                new ZoneEditPage(
                    zone,
                    _currentZoneIndex + 1));

            RenderZones();
            UpdateZoneInfo();
        }
        private void ShowAllObjects_Clicked(
    object? sender,
    EventArgs e)
        {
            if (GameObjectTypePicker.SelectedIndex != 0)
                return;

            ShowAllZones_Clicked(sender, e);
        }

        private void Hamburger_Clicked(
    object? sender,
    EventArgs e)
        {
            Shell.Current.FlyoutIsPresented = true;
        }


    }
}