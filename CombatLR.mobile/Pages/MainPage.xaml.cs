using System.Text.Json;
using CombatLR.mobile.Models;
using CombatLR.mobile.PageModels;
using CombatLR.mobile.Services;


namespace CombatLR.mobile.Pages
{
    public partial class MainPage : ContentPage
    {
        private MapDefinition? _mapDefinition;
        private MapCoordinateService? _coordinateService;
        private double _mapZoom = 1.0;
        private double _mapPanX = 0.0;
        private double _mapPanY = 0.0;
        private bool _isPinching;

        private const double MinMapZoom = 1.0;
        private const double MaxMapZoom = 8.0;

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



        public MainPage(MainPageModel model)
        {
            InitializeComponent();
            BindingContext = model;
#if ANDROID
            Loaded += MainPage_Loaded;
#endif

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
                    Path.Combine(
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
                    Path.Combine(
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

            MapInfoLabel.Text =
                $"MAP: {imageX:0.0}, {imageY:0.0}\n" +
                $"GPS:\n" +
                $"{gps.Latitude:F7}\n" +
                $"{gps.Longitude:F7}";
        }
        private void UpdateMapTransform()
        {
            MapImage.Scale = _mapZoom;

            MapImage.TranslationX = _mapPanX;
            MapImage.TranslationY = _mapPanY;
        }


        private void ResetMapView()
        {
            _mapZoom = 1.0;
            _mapPanX = 0.0;
            _mapPanY = 0.0;

            UpdateMapTransform();
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
    }
}