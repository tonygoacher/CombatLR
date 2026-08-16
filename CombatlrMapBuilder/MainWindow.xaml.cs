using Microsoft.Win32;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace CombatlrMapBuilder;

public partial class MainWindow : Window
{
    private const int OutputWidth = 1280;
    private const int OutputHeight = 960;
    private const double MinimumZoom = 0.10;
    private const double MaximumZoom = 4.00;
    private const double ZoomFactor = 1.15;

    private Point? _referencePointA;
    private Point? _referencePointB;

    private double _imageWidth;
    private double _imageHeight;
    private double _metresPerPixel;
    private double? _gpsDistanceMetres;
    private double? _gpsBearingDegrees;

    // Image-space -> local geographic east/north similarity transform.
    private double _eastMetresPerImagePixel;
    private double _northMetresPerImagePixel;
    private double _eastPerPixelX;
    private double _eastPerPixelY;
    private double _northPerPixelX;
    private double _northPerPixelY;
    private bool _transformValid;

    private double _playfieldLeft;
    private double _playfieldTop;

    private bool _draggingPlayfield;
    private bool _rightDraggingPlayfield;
    private Point _dragStartMouse;
    private double _playfieldStartLeft;
    private double _playfieldStartTop;

    private bool _resizingPlayfield;
    private double _resizeStartLeft;
    private double _resizeStartTop;
    private double _resizeStartWidth;
    private double _resizeStartHeight;
    private bool _resizeUsesWidth;

    private bool _initialised;
    private string? _projectFilePath;
    private string? _sourceImagePath;

    public MainWindow()
    {
        InitializeComponent();
        _initialised = true;
    }

    private void NewProject_Click(object sender, RoutedEventArgs e)
    {
        if (!ConfirmDiscardUnsavedProject())
            return;

        ResetProject();
        SetTitle();
    }

    private void OpenProject_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Title = "Open Combatlr MapBuilder project",
            Filter = "Combatlr projects|*.combatlr|JSON files|*.json|All files|*.*"
        };

        if (dialog.ShowDialog() != true)
            return;

        try
        {
            LoadProject(dialog.FileName);
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                this,
                $"Could not open the project.\n\n{ex.Message}",
                "Combatlr Map Builder",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void SaveProject_Click(object sender, RoutedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(_projectFilePath))
        {
            SaveProjectAs_Click(sender, e);
            return;
        }

        SaveProject(_projectFilePath);
    }

    private void SaveProjectAs_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new SaveFileDialog
        {
            Title = "Save Combatlr MapBuilder project",
            Filter = "Combatlr project|*.combatlr",
            DefaultExt = ".combatlr",
            AddExtension = true,
            FileName = string.IsNullOrWhiteSpace(_projectFilePath)
                ? "CombatlrMap.combatlr"
                : System.IO.Path.GetFileName(_projectFilePath)
        };

        if (dialog.ShowDialog() != true)
            return;

        SaveProject(dialog.FileName);
    }

    private bool ConfirmDiscardUnsavedProject()
    {
        // Stage 10 does not yet track a dirty flag. Keep this hook so the
        // project workflow can add one cleanly later.
        return true;
    }

    private void ResetProject()
    {
        _projectFilePath = null;
        _sourceImagePath = null;

        MapImage.Source = null;
        _imageWidth = 0;
        _imageHeight = 0;

        _referencePointA = null;
        _referencePointB = null;
        _gpsDistanceMetres = null;
        _gpsBearingDegrees = null;
        _transformValid = false;
        _metresPerPixel = 0;

        OverlayCanvas.Children.Clear();
        PlayfieldRectangle.Visibility = Visibility.Collapsed;

        MapScaleTransform.ScaleX = 1;
        MapScaleTransform.ScaleY = 1;
        ZoomStatus.Text = "100%";

        ImageStatus.Text = "No image loaded";
        PointStatus.Text = "Load an image, then click reference point A";
        ScaleStatus.Text = "Scale: —";
        DistanceDisplay.Text = "—";
        GpsStatus.Text = "GPS: —";
        TransformStatus.Text = "Transform: —";
        PlayfieldStatus.Text = "Site area: —";

        LatitudeATextBox.Clear();
        LongitudeATextBox.Clear();
        LatitudeBTextBox.Clear();
        LongitudeBTextBox.Clear();

        SetTitle();
    }

    private void SaveProject(string filePath)
    {
        var projectDirectory = System.IO.Path.GetDirectoryName(
            System.IO.Path.GetFullPath(filePath))!;

        var relativeImagePath = System.IO.Path.GetRelativePath(
            projectDirectory,
            System.IO.Path.GetFullPath(_sourceImagePath));

        var project = new ProjectMetadata
        {
            Version = 1,
            SourceImage = string.IsNullOrWhiteSpace(_sourceImagePath)
                ? null
                : System.IO.Path.GetRelativePath(
                    projectDirectory,
                    System.IO.Path.GetFullPath(_sourceImagePath)),
            RotationDegrees = ParseRotation(),
            Reference = _referencePointA is not null && _referencePointB is not null
                ? new ReferenceMetadata
                {
                    PointA = new PixelPoint
                    {
                        X = _referencePointA.Value.X,
                        Y = _referencePointA.Value.Y
                    },
                    PointB = new PixelPoint
                    {
                        X = _referencePointB.Value.X,
                        Y = _referencePointB.Value.Y
                    },
                    DistanceMetres = _gpsDistanceMetres ?? 0,
                    GpsA = TryReadGps(
                        LatitudeATextBox.Text,
                        LongitudeATextBox.Text,
                        out var saveLatA,
                        out var saveLonA)
                        ? new GpsPoint
                        {
                            Latitude = saveLatA,
                            Longitude = saveLonA
                        }
                        : null,
                    GpsB = TryReadGps(
                        LatitudeBTextBox.Text,
                        LongitudeBTextBox.Text,
                        out var saveLatB,
                        out var saveLonB)
                        ? new GpsPoint
                        {
                            Latitude = saveLatB,
                            Longitude = saveLonB
                        }
                        : null,
                    BearingDegrees = _gpsBearingDegrees ?? 0
                }
                : null,
            Playfield = new ProjectPlayfield
            {
                X = _playfieldLeft,
                Y = _playfieldTop,
                Width = PlayfieldRectangle.Width,
                Height = PlayfieldRectangle.Height
            }
        };

        var options = new JsonSerializerOptions
        {
            WriteIndented = true
        };

        File.WriteAllText(
            filePath,
            JsonSerializer.Serialize(project, options));

        _projectFilePath = filePath;
        SetTitle();
    }

    private void LoadProject(string filePath)
    {
        var json = File.ReadAllText(filePath);

        var project = JsonSerializer.Deserialize<ProjectMetadata>(json)
                      ?? throw new InvalidDataException("The project file is empty or invalid.");

        _projectFilePath = filePath;
        _sourceImagePath = null;

        ResetProjectStateForLoad();

        RotationTextBox.Text =
            project.RotationDegrees.ToString(
                CultureInfo.InvariantCulture);

        var projectDirectory = System.IO.Path.GetDirectoryName(
            System.IO.Path.GetFullPath(filePath))!;

        if (!string.IsNullOrWhiteSpace(project.SourceImage))
        {
            var sourceImage = System.IO.Path.GetFullPath(
                System.IO.Path.Combine(projectDirectory, project.SourceImage));

            if (!File.Exists(sourceImage))
            {
                throw new FileNotFoundException(
                    $"The project source image could not be found: {sourceImage}");
            }

            LoadSourceImage(sourceImage);
            _sourceImagePath = sourceImage;
        }

        if (project.Reference is not null)
        {
            _referencePointA = new Point(
                project.Reference.PointA.X,
                project.Reference.PointA.Y);

            _referencePointB = new Point(
                project.Reference.PointB.X,
                project.Reference.PointB.Y);

            if (project.Reference.GpsA is not null)
            {
                LatitudeATextBox.Text =
                    project.Reference.GpsA.Latitude.ToString(
                        CultureInfo.InvariantCulture);
                LongitudeATextBox.Text =
                    project.Reference.GpsA.Longitude.ToString(
                        CultureInfo.InvariantCulture);
            }

            if (project.Reference.GpsB is not null)
            {
                LatitudeBTextBox.Text =
                    project.Reference.GpsB.Latitude.ToString(
                        CultureInfo.InvariantCulture);
                LongitudeBTextBox.Text =
                    project.Reference.GpsB.Longitude.ToString(
                        CultureInfo.InvariantCulture);
            }

            if (MapImage.Source is not null)
                DrawReferenceLine();
        }

        if (MapImage.Source is not null)
        {
            UpdateScaleAndPlayfield();

            ResizePlayfield(
                project.Playfield.X,
                project.Playfield.Y,
                project.Playfield.Width,
                project.Playfield.Height);

            PlayfieldRectangle.Visibility = Visibility.Visible;
        }

        SetTitle();
    }

    private void ResetProjectStateForLoad()
    {
        MapImage.Source = null;
        _sourceImagePath = null;
        _imageWidth = 0;
        _imageHeight = 0;

        _referencePointA = null;
        _referencePointB = null;
        _gpsDistanceMetres = null;
        _gpsBearingDegrees = null;
        _transformValid = false;
        _metresPerPixel = 0;

        OverlayCanvas.Children.Clear();
        PlayfieldRectangle.Visibility = Visibility.Collapsed;

        LatitudeATextBox.Clear();
        LongitudeATextBox.Clear();
        LatitudeBTextBox.Clear();
        LongitudeBTextBox.Clear();

        ImageStatus.Text = "No image loaded";
        PointStatus.Text = "Load an image, then click reference point A";
        ScaleStatus.Text = "Scale: —";
        DistanceDisplay.Text = "—";
        GpsStatus.Text = "GPS: —";
        TransformStatus.Text = "Transform: —";
        PlayfieldStatus.Text = "Site area: —";
    }

    private void LoadSourceImage(string imagePath)
    {
        var bitmap = new BitmapImage();
        bitmap.BeginInit();
        bitmap.CacheOption = BitmapCacheOption.OnLoad;
        bitmap.UriSource = new Uri(imagePath);
        bitmap.EndInit();
        bitmap.Freeze();

        MapImage.Source = bitmap;

        _sourceImagePath = imagePath;
        _imageWidth = bitmap.PixelWidth;
        _imageHeight = bitmap.PixelHeight;

        MapWorkspace.Width = _imageWidth;
        MapWorkspace.Height = _imageHeight;
        OverlayCanvas.Width = _imageWidth;
        OverlayCanvas.Height = _imageHeight;

        MapScaleTransform.ScaleX = 1;
        MapScaleTransform.ScaleY = 1;
        ZoomStatus.Text = "100%";

        ImageStatus.Text =
            $"{System.IO.Path.GetFileName(imagePath)}  ({_imageWidth} × {_imageHeight})";

        PlayfieldRectangle.Visibility = Visibility.Visible;
    }

    private void SetTitle()
    {
        var projectName = string.IsNullOrWhiteSpace(_projectFilePath)
            ? "Untitled"
            : System.IO.Path.GetFileNameWithoutExtension(_projectFilePath);

        Title = $"Combatlr Map Builder - {projectName}";
    }

    private void OpenImage_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Title = "Open map image",
            Filter = "Image files|*.png;*.jpg;*.jpeg;*.bmp|All files|*.*"
        };

        if (dialog.ShowDialog() != true)
            return;

        try
        {
            LoadSourceImage(dialog.FileName);

            _projectFilePath = null;
            _referencePointA = null;
            _referencePointB = null;
            _gpsDistanceMetres = null;
            _gpsBearingDegrees = null;
            _transformValid = false;
            _metresPerPixel = 0;

            OverlayCanvas.Children.Clear();

            PointStatus.Text = "Click reference point A";
            ScaleStatus.Text = "Scale: —";
            DistanceDisplay.Text = "—";
            GpsStatus.Text = "GPS: —";
            TransformStatus.Text = "Transform: —";
            PlayfieldStatus.Text = "Site area: —";

            _playfieldLeft = Math.Max(0, (_imageWidth - 1280) / 2);
            _playfieldTop = Math.Max(0, (_imageHeight - 960) / 2);
            PlayfieldRectangle.Width = 1280;
            PlayfieldRectangle.Height = 960;
            SetPlayfieldPosition(_playfieldLeft, _playfieldTop);

            SetTitle();
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                this,
                $"Could not load the image.\n\n{ex.Message}",
                "Combatlr Map Builder",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void MapImage_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (MapImage.Source is null)
            return;

        var position = e.GetPosition(MapWorkspace);

        if (_referencePointA is null)
        {
            _referencePointA = position;
            _referencePointB = null;
            PointStatus.Text =
                $"A = ({position.X:0.0}, {position.Y:0.0}) — click reference point B";
        }
        else if (_referencePointB is null)
        {
            _referencePointB = position;
            PointStatus.Text =
                $"A = ({_referencePointA.Value.X:0.0}, {_referencePointA.Value.Y:0.0})   " +
                $"B = ({position.X:0.0}, {position.Y:0.0})";
        }
        else
        {
            _referencePointA = position;
            _referencePointB = null;
            PointStatus.Text =
                $"A = ({position.X:0.0}, {position.Y:0.0}) — click reference point B";
        }

        DrawReferenceLine();
        UpdateScaleAndPlayfield();
        e.Handled = true;
    }

    private void ClearPoints_Click(object sender, RoutedEventArgs e)
    {
        _referencePointA = null;
        _referencePointB = null;
        OverlayCanvas.Children.Clear();

        PointStatus.Text = MapImage.Source is null
            ? "No image loaded"
            : "Click reference point A";

        ScaleStatus.Text = "Scale: —";
        GpsStatus.Text = "GPS: —";
        PlayfieldStatus.Text = "Playfield: —";
        _gpsDistanceMetres = null;
        _gpsBearingDegrees = null;

        if (MapImage.Source is not null)
            PlayfieldRectangle.Visibility = Visibility.Visible;
    }

    private void SettingsTextBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (_initialised)
            UpdateScaleAndPlayfield();
    }

    private void GpsTextBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (_initialised)
            UpdateScaleAndPlayfield();
    }

    private void UpdateScaleAndPlayfield()
    {
        if (_referencePointA is null || _referencePointB is null)
        {
            ScaleStatus.Text = "Scale: —";
            DistanceDisplay.Text = "—";
            PlayfieldStatus.Text = "Playfield: —";
            TransformStatus.Text = "Transform: —";
            return;
        }

        UpdateGpsCalibration();

        if (!_transformValid || !_gpsDistanceMetres.HasValue)
        {
            ScaleStatus.Text = "Scale: enter valid GPS coordinates for A and B";
            PlayfieldStatus.Text = "Playfield: —";
            return;
        }

        _metresPerPixel =
            _gpsDistanceMetres.Value /
            Math.Sqrt(
                Math.Pow(_referencePointB.Value.X - _referencePointA.Value.X, 2) +
                Math.Pow(_referencePointB.Value.Y - _referencePointA.Value.Y, 2));

        ScaleStatus.Text =
            $"Scale: {_metresPerPixel:0.000000} m/pixel";

        DistanceDisplay.Text =
            $"{_gpsDistanceMetres.Value:0.00} m";

        UpdatePlayfieldStatus();

        SetPlayfieldPhysicalSize();
    }

    private void UpdatePlayfieldStatus()
    {
        if (!_transformValid || _metresPerPixel <= 0)
        {
            PlayfieldStatus.Text = "Playfield: —";
            return;
        }

        var width = PlayfieldRectangle.Width;
        var height = PlayfieldRectangle.Height;

        PlayfieldStatus.Text =
            $"Site area: {width:0}×{height:0} px = " +
            $"{width * _metresPerPixel:0.0}×{height * _metresPerPixel:0.0} m";
    }

    private void UpdateGpsCalibration()
    {
        _transformValid = false;
        _gpsDistanceMetres = null;
        _gpsBearingDegrees = null;

        if (!TryReadGps(
                LatitudeATextBox.Text, LongitudeATextBox.Text,
                out var latA, out var lonA) ||
            !TryReadGps(
                LatitudeBTextBox.Text, LongitudeBTextBox.Text,
                out var latB, out var lonB))
        {
            GpsStatus.Text = "GPS: enter valid A/B coordinates";
            TransformStatus.Text = "Transform: —";
            return;
        }

        if (_referencePointA is null || _referencePointB is null)
        {
            GpsStatus.Text = "GPS: A/B entered — select image A/B";
            TransformStatus.Text = "Transform: —";
            return;
        }

        _gpsDistanceMetres =
            HaversineDistanceMetres(latA, lonA, latB, lonB);

        _gpsBearingDegrees =
            InitialBearingDegrees(latA, lonA, latB, lonB);

        BuildImageToGeoTransform(
            _referencePointA.Value,
            _referencePointB.Value,
            latA, lonA, latB, lonB);

        GpsStatus.Text =
            $"GPS: {_gpsDistanceMetres.Value:0.00} m, bearing {_gpsBearingDegrees.Value:0.0}°";

        TransformStatus.Text =
            $"Transform: {_transformRotationDegrees:0.0}°";
    }

    private double _transformRotationDegrees;

    private void BuildImageToGeoTransform(
    Point imageA,
    Point imageB,
    double latA,
    double lonA,
    double latB,
    double lonB)
    {
        // Local equirectangular projection around A.
        var meanLat = ((latA + latB) / 2.0) * Math.PI / 180.0;
        const double earthRadius = 6371008.8;

        var dLon = (lonB - lonA) * Math.PI / 180.0;
        var dLat = (latB - latA) * Math.PI / 180.0;

        var east = dLon * Math.Cos(meanLat) * earthRadius;
        var north = dLat * earthRadius;

        var dx = imageB.X - imageA.X;
        var dy = imageB.Y - imageA.Y;

        var pixelDistance = Math.Sqrt(dx * dx + dy * dy);
        var geoDistance = Math.Sqrt(east * east + north * north);

        if (pixelDistance < 0.000001 || geoDistance < 0.000001)
            return;

        // Unit vector along A -> B in image coordinates.
        var ix = dx / pixelDistance;
        var iy = dy / pixelDistance;

        // Unit vector along A -> B in geographic east/north coordinates.
        var gx = east / geoDistance;
        var gy = north / geoDistance;

        var scale = geoDistance / pixelDistance;

        // Complete 2-D similarity transform.
        //
        // The first column maps image +X.
        // The second column maps image +Y.
        //
        // This preserves the A -> B calibration while also
        // correctly transforming movement perpendicular to A -> B.

        _eastPerPixelX =
            scale * (ix * gx + iy * gy);

        _eastPerPixelY =
            scale * (iy * gx - ix * gy);

        _northPerPixelX =
            scale * (ix * gy - iy * gx);

        _northPerPixelY =
            scale * (iy * gy + ix * gx);

        // Bearing of the image +X axis.
        var imageAxisEast = _eastPerPixelX;
        var imageAxisNorth = _northPerPixelX;

        _transformRotationDegrees =
            NormalizeDegrees(
                Math.Atan2(
                    imageAxisEast,
                    imageAxisNorth) *
                180.0 / Math.PI);

        _transformValid = true;
    }

    private GeoPoint ImageToGeo(Point imagePoint)
    {
        if (!_transformValid || _referencePointA is null)
            throw new InvalidOperationException("Map calibration is incomplete.");

        if (!TryReadGps(
                LatitudeATextBox.Text, LongitudeATextBox.Text,
                out var latA, out var lonA))
            throw new InvalidOperationException("GPS calibration is incomplete.");

        var dx = imagePoint.X - _referencePointA.Value.X;
        var dy = imagePoint.Y - _referencePointA.Value.Y;

        var east = _eastPerPixelX * dx + _eastPerPixelY * dy;
        var north = _northPerPixelX * dx + _northPerPixelY * dy;

        const double earthRadius = 6371008.8;
        var lat = latA + (north / earthRadius) * 180.0 / Math.PI;
        var lon = lonA +
                  (east / (earthRadius * Math.Cos(latA * Math.PI / 180.0))) *
                  180.0 / Math.PI;

        return new GeoPoint(lat, lon);
    }

    private static bool TryReadGps(
        string latitudeText,
        string longitudeText,
        out double latitude,
        out double longitude)
    {
        latitude = 0;
        longitude = 0;

        if (!double.TryParse(
                latitudeText,
                NumberStyles.Float,
                CultureInfo.InvariantCulture,
                out latitude) ||
            !double.TryParse(
                longitudeText,
                NumberStyles.Float,
                CultureInfo.InvariantCulture,
                out longitude))
            return false;

        return latitude >= -90 && latitude <= 90 &&
               longitude >= -180 && longitude <= 180;
    }

    private static double HaversineDistanceMetres(
        double latitudeA,
        double longitudeA,
        double latitudeB,
        double longitudeB)
    {
        const double earthRadiusMetres = 6371008.8;

        var lat1 = latitudeA * Math.PI / 180.0;
        var lat2 = latitudeB * Math.PI / 180.0;
        var dLat = (latitudeB - latitudeA) * Math.PI / 180.0;
        var dLon = (longitudeB - longitudeA) * Math.PI / 180.0;

        var sinLat = Math.Sin(dLat / 2);
        var sinLon = Math.Sin(dLon / 2);

        var a = sinLat * sinLat +
                Math.Cos(lat1) * Math.Cos(lat2) * sinLon * sinLon;

        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));

        return earthRadiusMetres * c;
    }

    private static double InitialBearingDegrees(
        double latitudeA,
        double longitudeA,
        double latitudeB,
        double longitudeB)
    {
        var lat1 = latitudeA * Math.PI / 180.0;
        var lat2 = latitudeB * Math.PI / 180.0;
        var dLon = (longitudeB - longitudeA) * Math.PI / 180.0;

        var y = Math.Sin(dLon) * Math.Cos(lat2);
        var x = Math.Cos(lat1) * Math.Sin(lat2) -
                Math.Sin(lat1) * Math.Cos(lat2) * Math.Cos(dLon);

        return NormalizeDegrees(Math.Atan2(y, x) * 180.0 / Math.PI);
    }

    private static double NormalizeDegrees(double degrees)
    {
        degrees %= 360.0;
        if (degrees < 0)
            degrees += 360.0;
        return degrees;
    }

    private void SetPlayfieldPhysicalSize()
    {
        if (_metresPerPixel <= 0)
            return;

        // Output pixels are converted to source-image pixels using the
        // calibrated scale.  The source image is assumed to be at its
        // original pixel density.
        //
        // This means the selected source rectangle represents the same
        // real-world area as a 1280x960 Combatlr image.
        //
        // For Stage 2 we keep the output crop dimensions fixed in the
        // source image as a practical first implementation.  Export uses
        // the selected area and resamples it to 1280x960.
        var sourceWidth = OutputWidth;
        var sourceHeight = OutputHeight;

        if (_imageWidth < sourceWidth || _imageHeight < sourceHeight)
            return;

        _playfieldLeft = Math.Max(0, Math.Min(
            _playfieldLeft, _imageWidth - sourceWidth));

        _playfieldTop = Math.Max(0, Math.Min(
            _playfieldTop, _imageHeight - sourceHeight));

        SetPlayfieldPosition(_playfieldLeft, _playfieldTop);
    }

    private void SetPlayfieldPosition(double left, double top)
    {
        var currentWidth = PlayfieldRectangle.Width;
        var currentHeight = PlayfieldRectangle.Height;

        if (_imageWidth >= currentWidth)
            left = Math.Max(0, Math.Min(left, _imageWidth - currentWidth));
        else
            left = 0;

        if (_imageHeight >= currentHeight)
            top = Math.Max(0, Math.Min(top, _imageHeight - currentHeight));
        else
            top = 0;

        _playfieldLeft = left;
        _playfieldTop = top;

        Canvas.SetLeft(PlayfieldRectangle, left);
        Canvas.SetTop(PlayfieldRectangle, top);

        if (_imageWidth < OutputWidth || _imageHeight < OutputHeight)
        {
            PlayfieldStatus.Text =
                "Playfield: source image is smaller than 1280×960";
        }
    }

    private void DrawReferenceLine()
    {
        OverlayCanvas.Children.Clear();

        if (_referencePointA is not null)
            AddMarker(_referencePointA.Value, "A");

        if (_referencePointB is null)
            return;

        AddMarker(_referencePointB.Value, "B");

        var line = new Line
        {
            X1 = _referencePointA!.Value.X,
            Y1 = _referencePointA.Value.Y,
            X2 = _referencePointB.Value.X,
            Y2 = _referencePointB.Value.Y,
            Stroke = Brushes.Red,
            StrokeThickness = 2
        };

        OverlayCanvas.Children.Add(line);
    }

    private void AddMarker(Point point, string label)
    {
        const double size = 12;

        var ellipse = new Ellipse
        {
            Width = size,
            Height = size,
            Fill = Brushes.Red,
            Stroke = Brushes.White,
            StrokeThickness = 2
        };

        Canvas.SetLeft(ellipse, point.X - size / 2);
        Canvas.SetTop(ellipse, point.Y - size / 2);
        OverlayCanvas.Children.Add(ellipse);

        var text = new TextBlock
        {
            Text = label,
            Foreground = Brushes.White,
            FontWeight = FontWeights.Bold,
            Background = Brushes.Black,
            Padding = new Thickness(3, 1, 3, 1)
        };

        Canvas.SetLeft(text, point.X + 8);
        Canvas.SetTop(text, point.Y - 12);
        OverlayCanvas.Children.Add(text);
    }

    private void Playfield_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (MapImage.Source is null || _resizingPlayfield)
            return;

        StartPlayfieldDrag(e);
        e.Handled = true;
    }

    private void Playfield_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (MapImage.Source is null || _resizingPlayfield)
            return;

        // Right-click is the primary "move playfield" gesture. Suppress the
        // normal WPF context menu behaviour.
        StartPlayfieldDrag(e);
        e.Handled = true;
    }

    private void StartPlayfieldDrag(MouseButtonEventArgs e)
    {
        _dragStartMouse = e.GetPosition(MapWorkspace);
        _playfieldStartLeft = _playfieldLeft;
        _playfieldStartTop = _playfieldTop;

        if (e.ChangedButton == MouseButton.Right)
        {
            _rightDraggingPlayfield = true;
            PlayfieldRectangle.CaptureMouse();
        }
        else
        {
            _draggingPlayfield = true;
            PlayfieldRectangle.CaptureMouse();
        }
    }

    private void Playfield_MouseMove(object sender, MouseEventArgs e)
    {
        if (!_draggingPlayfield && !_rightDraggingPlayfield)
            return;

        var current = e.GetPosition(MapWorkspace);
        var dx = current.X - _dragStartMouse.X;
        var dy = current.Y - _dragStartMouse.Y;

        SetPlayfieldPosition(
            _playfieldStartLeft + dx,
            _playfieldStartTop + dy);

        e.Handled = true;
    }

    private void Playfield_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (!_draggingPlayfield)
            return;

        _draggingPlayfield = false;

        if (!_rightDraggingPlayfield)
            PlayfieldRectangle.ReleaseMouseCapture();

        e.Handled = true;
    }

    private void Playfield_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (!_rightDraggingPlayfield)
            return;

        _rightDraggingPlayfield = false;

        if (!_draggingPlayfield)
            PlayfieldRectangle.ReleaseMouseCapture();

        e.Handled = true;
    }

    private void ResizeHandle_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
    {
        if (_resizingPlayfield || MapImage.Source is null)
            return;

        _resizingPlayfield = true;
        _resizeStartLeft = _playfieldLeft;
        _resizeStartTop = _playfieldTop;
        _resizeStartWidth = PlayfieldRectangle.Width;
        _resizeStartHeight = PlayfieldRectangle.Height;

        // Decide once, at the beginning of the drag, whether horizontal or
        // vertical movement controls the resize. This prevents the rectangle
        // from jumping between widths as individual DragDelta events vary.
        var mouse = Mouse.GetPosition(MapWorkspace);
        var handle = ReferenceEquals(sender, TopLeftHandle)
            ? new Point(_resizeStartLeft, _resizeStartTop)
            : new Point(
                _resizeStartLeft + _resizeStartWidth,
                _resizeStartTop + _resizeStartHeight);

        var dx = mouse.X - handle.X;
        var dy = mouse.Y - handle.Y;

        _resizeUsesWidth = Math.Abs(dx) >= Math.Abs(dy);

        e.Handled = true;
    }

    private void ResizeHandle_DragDelta(object sender, System.Windows.Controls.Primitives.DragDeltaEventArgs e)
    {
        if (!_resizingPlayfield || MapImage.Source is null)
            return;

        const double aspectRatio = OutputWidth / OutputHeight; // 4:3
        const double minimumWidth = 320;

        if (ReferenceEquals(sender, TopLeftHandle))
        {
            double width;

            if (_resizeUsesWidth)
            {
                width = _resizeStartWidth - e.HorizontalChange;
            }
            else
            {
                var newHeight = _resizeStartHeight - e.VerticalChange;
                width = newHeight * aspectRatio;
            }

            width = Math.Max(minimumWidth, width);
            var height = width / aspectRatio;

            var right = _resizeStartLeft + _resizeStartWidth;
            var bottom = _resizeStartTop + _resizeStartHeight;

            ResizePlayfield(
                right - width,
                bottom - height,
                width,
                height);
        }
        else
        {
            double width;

            if (_resizeUsesWidth)
            {
                width = _resizeStartWidth + e.HorizontalChange;
            }
            else
            {
                var newHeight = _resizeStartHeight + e.VerticalChange;
                width = newHeight * aspectRatio;
            }

            width = Math.Max(minimumWidth, width);
            var height = width / aspectRatio;

            ResizePlayfield(
                _resizeStartLeft,
                _resizeStartTop,
                width,
                height);
        }

        e.Handled = true;
    }

    private void ResizeHandle_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
    {
        _resizingPlayfield = false;
        e.Handled = true;
    }

    private void ResizePlayfield(double left, double top, double width, double height)
    {
        if (MapImage.Source is null)
            return;

        const double minimumWidth = 320;
        const double aspectRatio = OutputWidth / OutputHeight; // 4:3

        width = Math.Max(minimumWidth, width);
        height = width / aspectRatio;

        // If the requested size cannot fit, reduce it while preserving 4:3.
        if (width > _imageWidth)
        {
            width = _imageWidth;
            height = width / aspectRatio;
        }

        if (height > _imageHeight)
        {
            height = _imageHeight;
            width = height * aspectRatio;
        }

        // Clamp the fixed/opposite corner into the image without changing
        // the aspect ratio.
        left = Math.Max(0, Math.Min(left, _imageWidth - width));
        top = Math.Max(0, Math.Min(top, _imageHeight - height));

        PlayfieldRectangle.Width = width;
        PlayfieldRectangle.Height = height;

        SetPlayfieldPosition(left, top);

        var widthMetres = width * _metresPerPixel;
        var heightMetres = height * _metresPerPixel;

        UpdatePlayfieldStatus();
    }

    private void MapScrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (MapImage.Source is null)
            return;

        var oldZoom = MapScaleTransform.ScaleX;
        var newZoom = e.Delta > 0
            ? oldZoom * ZoomFactor
            : oldZoom / ZoomFactor;

        newZoom = Math.Max(MinimumZoom, Math.Min(MaximumZoom, newZoom));

        if (Math.Abs(newZoom - oldZoom) < 0.0001)
            return;

        // MapWorkspace is top-left aligned. Therefore the image coordinate
        // underneath the mouse is simply the scroll offset plus mouse position,
        // divided by the current zoom.
        var mouse = e.GetPosition(MapScrollViewer);

        var imageX = (MapScrollViewer.HorizontalOffset + mouse.X) / oldZoom;
        var imageY = (MapScrollViewer.VerticalOffset + mouse.Y) / oldZoom;

        MapScaleTransform.ScaleX = newZoom;
        MapScaleTransform.ScaleY = newZoom;

        ZoomStatus.Text = $"{newZoom * 100:0}%";

        // LayoutTransform updates the ScrollViewer extent asynchronously.
        Dispatcher.BeginInvoke(new Action(() =>
        {
            MapScrollViewer.ScrollToHorizontalOffset(
                imageX * newZoom - mouse.X);

            MapScrollViewer.ScrollToVerticalOffset(
                imageY * newZoom - mouse.Y);
        }), System.Windows.Threading.DispatcherPriority.Loaded);

        e.Handled = true;
    }

    private void ExportMap_Click(object sender, RoutedEventArgs e)
    {
        if (MapImage.Source is null || string.IsNullOrWhiteSpace(_sourceImagePath) ||
            _referencePointA is null || _referencePointB is null || !_transformValid || _metresPerPixel <= 0)
        {
            MessageBox.Show(this, "A source image and complete A/B GPS calibration are required before exporting a site package.", "Combatlr Map Builder", MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }

        var dialog = new Microsoft.Win32.SaveFileDialog
        {
            Title = "Export Combatlr site package",
            Filter = "RGB565 BMP|*.bmp",
            DefaultExt = ".bmp",
            AddExtension = true,
            FileName = "map.bmp"
        };
        if (dialog.ShowDialog() != true) return;

        try
        {
            var outputBmpPath = dialog.FileName;
            var outputDirectory = System.IO.Path.GetDirectoryName(System.IO.Path.GetFullPath(outputBmpPath))!;
            var outputJsonPath = System.IO.Path.Combine(outputDirectory, "map.json");

            WriteRgb565Bitmap(_sourceImagePath, outputBmpPath, _playfieldLeft, _playfieldTop,
                PlayfieldRectangle.Width, PlayfieldRectangle.Height);

            var cropWidth = PlayfieldRectangle.Width;
            var cropHeight = PlayfieldRectangle.Height;
            var cropTopLeft = ImageToGeo(new System.Windows.Point(_playfieldLeft, _playfieldTop));
            var cropTopRight = ImageToGeo(new System.Windows.Point(_playfieldLeft + cropWidth, _playfieldTop));
            var cropBottomLeft = ImageToGeo(new System.Windows.Point(_playfieldLeft, _playfieldTop + cropHeight));
            var cropBottomRight = ImageToGeo(new System.Windows.Point(_playfieldLeft + cropWidth, _playfieldTop + cropHeight));
            var cropCentre = ImageToGeo(new System.Windows.Point(_playfieldLeft + cropWidth / 2.0, _playfieldTop + cropHeight / 2.0));

            var metadata = new MapMetadata
            {
                Version = 1,
                Image = new ImageMetadata { Width = OutputWidth, Height = OutputHeight },
                RotationDegrees = ParseRotation(),
                MetresPerPixel = _metresPerPixel,
                SourceCrop = new CropMetadata { X = _playfieldLeft, Y = _playfieldTop, Width = cropWidth, Height = cropHeight },
                Playfield = new PlayfieldMetadata
                {
                    Centre = new GpsPoint { Latitude = cropCentre.Latitude, Longitude = cropCentre.Longitude },
                    TopLeft = new GpsPoint { Latitude = cropTopLeft.Latitude, Longitude = cropTopLeft.Longitude },
                    TopRight = new GpsPoint { Latitude = cropTopRight.Latitude, Longitude = cropTopRight.Longitude },
                    BottomLeft = new GpsPoint { Latitude = cropBottomLeft.Latitude, Longitude = cropBottomLeft.Longitude },
                    BottomRight = new GpsPoint { Latitude = cropBottomRight.Latitude, Longitude = cropBottomRight.Longitude }
                },
                Reference = new ReferenceMetadata
                {
                    PointA = new PixelPoint { X = _referencePointA.Value.X, Y = _referencePointA.Value.Y },
                    PointB = new PixelPoint { X = _referencePointB.Value.X, Y = _referencePointB.Value.Y },
                    DistanceMetres = _gpsDistanceMetres!.Value,
                    GpsA = new GpsPoint { Latitude = double.Parse(LatitudeATextBox.Text, CultureInfo.InvariantCulture), Longitude = double.Parse(LongitudeATextBox.Text, CultureInfo.InvariantCulture) },
                    GpsB = new GpsPoint { Latitude = double.Parse(LatitudeBTextBox.Text, CultureInfo.InvariantCulture), Longitude = double.Parse(LongitudeBTextBox.Text, CultureInfo.InvariantCulture) },
                    BearingDegrees = _gpsBearingDegrees!.Value
                }
            };
            var options = new JsonSerializerOptions { WriteIndented = true };
            File.WriteAllText(outputJsonPath, JsonSerializer.Serialize(metadata, options));
            MessageBox.Show(this, $"Site package exported successfully.\n\n{System.IO.Path.GetFileName(outputBmpPath)}\n{System.IO.Path.GetFileName(outputJsonPath)}", "Combatlr Map Builder", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not export the site package.\n\n{ex.Message}", "Combatlr Map Builder", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private static void WriteRgb565Bitmap(string sourcePath, string outputPath, double cropX, double cropY, double cropWidth, double cropHeight)
    {
        using var source = new System.Drawing.Bitmap(sourcePath, true);
        using var output = new System.Drawing.Bitmap(OutputWidth, OutputHeight, System.Drawing.Imaging.PixelFormat.Format16bppRgb565);
        using (var graphics = System.Drawing.Graphics.FromImage(output))
        {
            graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
            graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
            graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
            graphics.CompositingQuality = System.Drawing.Drawing2D.CompositingQuality.HighQuality;
            graphics.DrawImage(source, new System.Drawing.Rectangle(0, 0, OutputWidth, OutputHeight),
                new System.Drawing.RectangleF((float)cropX, (float)cropY, (float)cropWidth, (float)cropHeight),
                System.Drawing.GraphicsUnit.Pixel);
        }
        WriteRgb565BmpFile(output, outputPath);
    }

    private static void WriteRgb565BmpFile(System.Drawing.Bitmap bitmap, string outputPath)
    {
        const int bitsPerPixel = 16;
        const int compressionBitfields = 3;
        const int infoHeaderSize = 40;
        const int maskSize = 12;
        var width = bitmap.Width;
        var height = bitmap.Height;
        var rowStride = ((width * bitsPerPixel + 31) / 32) * 4;
        var pixelDataSize = rowStride * height;
        var pixelOffset = 14 + infoHeaderSize + maskSize;
        var fileSize = pixelOffset + pixelDataSize;

        var data = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, width, height),
            System.Drawing.Imaging.ImageLockMode.ReadOnly,
            System.Drawing.Imaging.PixelFormat.Format16bppRgb565);
        try
        {
            using var stream = new FileStream(outputPath, FileMode.Create, FileAccess.Write, FileShare.None);
            using var writer = new BinaryWriter(stream);
            writer.Write((byte)'B'); writer.Write((byte)'M'); writer.Write(fileSize);
            writer.Write((short)0); writer.Write((short)0); writer.Write(pixelOffset);
            writer.Write(infoHeaderSize); writer.Write(width); writer.Write(-height);
            writer.Write((short)1); writer.Write((short)bitsPerPixel); writer.Write(compressionBitfields);
            writer.Write(pixelDataSize); writer.Write(0); writer.Write(0); writer.Write(0); writer.Write(0);
            writer.Write(0xF800); writer.Write(0x07E0); writer.Write(0x001F);

            var sourceStride = Math.Abs(data.Stride);
            var row = new byte[sourceStride];
            for (var y = 0; y < height; y++)
            {
                var sourceOffset = y * sourceStride;
                System.Runtime.InteropServices.Marshal.Copy(data.Scan0 + sourceOffset, row, 0, sourceStride);
                writer.Write(row, 0, width * 2);
                for (var padding = width * 2; padding < rowStride; padding++) writer.Write((byte)0);
            }
        }
        finally { bitmap.UnlockBits(data); }
    }

    private double ParseRotation()
    {
        return double.TryParse(
            RotationTextBox.Text,
            NumberStyles.Float,
            CultureInfo.InvariantCulture,
            out var rotation)
            ? rotation
            : 0;
    }

    private readonly record struct GeoPoint(double Latitude, double Longitude);

    private sealed class ProjectMetadata
    {
        public int Version { get; set; }
        public string? SourceImage { get; set; }
        public double RotationDegrees { get; set; }
        public ReferenceMetadata? Reference { get; set; }
        public ProjectPlayfield Playfield { get; set; } = new();
    }

    private sealed class ProjectPlayfield
    {
        public double X { get; set; }
        public double Y { get; set; }
        public double Width { get; set; } = 1280;
        public double Height { get; set; } = 960;
    }

    private sealed class MapMetadata
    {
        public int Version { get; set; }
        public ImageMetadata Image { get; set; } = new();
        public double RotationDegrees { get; set; }
        public double MetresPerPixel { get; set; }
        public CropMetadata SourceCrop { get; set; } = new();
        public PlayfieldMetadata Playfield { get; set; } = new();
        public ReferenceMetadata Reference { get; set; } = new();
    }

    private sealed class ImageMetadata
    {
        public int Width { get; set; }
        public int Height { get; set; }
    }

    private sealed class CropMetadata
    {
        public double X { get; set; }
        public double Y { get; set; }
        public double Width { get; set; }
        public double Height { get; set; }
    }

    private sealed class PlayfieldMetadata
    {
        public GpsPoint Centre { get; set; } = new();
        public GpsPoint TopLeft { get; set; } = new();
        public GpsPoint TopRight { get; set; } = new();
        public GpsPoint BottomLeft { get; set; } = new();
        public GpsPoint BottomRight { get; set; } = new();
    }

    private sealed class ReferenceMetadata
    {
        public PixelPoint PointA { get; set; } = new();
        public PixelPoint PointB { get; set; } = new();
        public double DistanceMetres { get; set; }
        public GpsPoint? GpsA { get; set; }
        public GpsPoint? GpsB { get; set; }
        public double BearingDegrees { get; set; }
    }

    private sealed class GpsPoint
    {
        public double Latitude { get; set; }
        public double Longitude { get; set; }
    }

    private sealed class PixelPoint
    {
        public double X { get; set; }
        public double Y { get; set; }
    }
}
