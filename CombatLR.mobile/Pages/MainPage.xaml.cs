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

        public MainPage(MainPageModel model)
        {
            InitializeComponent();
            BindingContext = model;

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

            // The Image uses AspectFit, so calculate
            // the actual displayed bitmap rectangle.
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

            var imageX =
                (point.X - imageLeft) / scale;

            var imageY =
                (point.Y - imageTop) / scale;

            // Ignore taps outside the actual bitmap.
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

    }
}