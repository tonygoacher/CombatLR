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

        private void Map_Tapped(
      object? sender,
      TappedEventArgs e)
        {
            if (_mapDefinition is null ||
                _coordinateService is null)
                return;

            if (sender is not VisualElement view)
                return;

            var points = e.GetPosition(view);

            if (points is null)
                return;

            var point = points.Value;

            var imageWidth =
                _mapDefinition.Image.Width;

            var imageHeight =
                _mapDefinition.Image.Height;

            var areaWidth =
                view.Width;

            var areaHeight =
                view.Height;

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

            // Undo pan and zoom.
            var unpannedX =
                point.X - _mapPanX;

            var unpannedY =
                point.Y - _mapPanY;

            // MapImage scales around its centre.
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

            if (imageX < 0 ||
                imageY < 0 ||
                imageX > imageWidth ||
                imageY > imageHeight)
            {
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

            private double _startDistance;
            private double _startZoom;

            private double _startPanX;
            private double _startPanY;

            private double _startFocusX;
            private double _startFocusY;

            private double _lastX;
            private double _lastY;

            public MapTouchListener(MainPage page)
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

                switch (e.ActionMasked)
                {
                    case Android.Views.MotionEventActions.Down:

                        _pinching = false;

                        _lastX =
                            e.GetX(0) / density;

                        _lastY =
                            e.GetY(0) / density;

                        break;


                    case Android.Views.MotionEventActions.PointerDown:

                        if (e.PointerCount >= 2)
                        {
                            _pinching = true;

                            _startDistance =
                                GetDistance(e, density);

                            _startZoom =
                                _page._mapZoom;

                            _startPanX =
                                _page._mapPanX;

                            _startPanY =
                                _page._mapPanY;

                            _startFocusX =
                                GetFocusX(e, density);

                            _startFocusY =
                                GetFocusY(e, density);
                        }

                        break;


                    case Android.Views.MotionEventActions.Move:

                        if (_pinching &&
                            e.PointerCount >= 2)
                        {
                            var distance =
                                GetDistance(e, density);

                            if (_startDistance > 0)
                            {
                                var rawRatio = distance / _startDistance;

                                var zoomRatio = 1.0 + (rawRatio - 1.0) * 0.35;

                                var newZoom =
                                    Math.Clamp(
                                        _startZoom * zoomRatio,
                                        MinMapZoom,
                                        MaxMapZoom);

                                // Keep the point under the fingers
                                // stationary while zooming.
                                var centreX =
                                    _page.MapContainer.Width / 2.0;

                                var centreY =
                                    _page.MapContainer.Height / 2.0;

                                var focusX =
                                    GetFocusX(e, density);

                                var focusY =
                                    GetFocusY(e, density);

                                var ratio =
                                    newZoom / _startZoom;

                                _page._mapPanX =
                                    (1.0 - ratio) *
                                    (focusX - centreX) +
                                    ratio *
                                    _startPanX;

                                _page._mapPanY =
                                    (1.0 - ratio) *
                                    (focusY - centreY) +
                                    ratio *
                                    _startPanY;

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

                            _page._mapPanX +=
                                x - _lastX;

                            _page._mapPanY +=
                                y - _lastY;

                            _lastX = x;
                            _lastY = y;

                            _page.UpdateMapTransform();
                        }

                        break;


                    case Android.Views.MotionEventActions.PointerUp:

                        if (e.PointerCount <= 2)
                            _pinching = false;

                        break;


                    case Android.Views.MotionEventActions.Up:
                    case Android.Views.MotionEventActions.Cancel:

                        _pinching = false;

                        break;
                }

                // IMPORTANT:
                // Returning false allows the normal MAUI tap
                // recognizer to continue receiving taps.
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

            private static double GetFocusX(
                Android.Views.MotionEvent e,
                double density)
            {
                return (
                    e.GetX(0) +
                    e.GetX(1)) /
                    2.0 /
                    density;
            }

            private static double GetFocusY(
                Android.Views.MotionEvent e,
                double density)
            {
                return (
                    e.GetY(0) +
                    e.GetY(1)) /
                    2.0 /
                    density;
            }
        }

#endif
    }
}