using Microsoft.Win32;
using System.Globalization;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Controls;

namespace CombatlrMapBuilder;

public partial class MainWindow : Window
{
    private const double OutputWidth = 1280;
    private const double OutputHeight = 960;

    private Point? _referencePointA;
    private Point? _referencePointB;

    private double _imageWidth;
    private double _imageHeight;

    private bool _draggingPlayfield;
    private Point _dragStartMouse;
    private double _playfieldStartLeft;
    private double _playfieldStartTop;
    private bool _initialised = false;

    public MainWindow()
    {
        InitializeComponent();
        _initialised = true;
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
            var bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.CacheOption = BitmapCacheOption.OnLoad;
            bitmap.UriSource = new Uri(dialog.FileName);
            bitmap.EndInit();
            bitmap.Freeze();

            MapImage.Source = bitmap;

            _imageWidth = bitmap.PixelWidth;
            _imageHeight = bitmap.PixelHeight;

            MapWorkspace.Width = _imageWidth;
            MapWorkspace.Height = _imageHeight;

            OverlayCanvas.Width = _imageWidth;
            OverlayCanvas.Height = _imageHeight;

            _referencePointA = null;
            _referencePointB = null;
            OverlayCanvas.Children.Clear();

            ImageStatus.Text =
                $"{System.IO.Path.GetFileName(dialog.FileName)}  ({_imageWidth} × {_imageHeight})";

            PointStatus.Text = "Click reference point A";
            ScaleStatus.Text = "Scale: —";
            PlayfieldStatus.Text = "Playfield: —";

            PlayfieldRectangle.Visibility = Visibility.Collapsed;
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

        var position = e.GetPosition(MapImage);

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

            CreatePlayfield();
        }
        else
        {
            _referencePointA = position;
            _referencePointB = null;
            PointStatus.Text =
                $"A = ({position.X:0.0}, {position.Y:0.0}) — click reference point B";

            PlayfieldRectangle.Visibility = Visibility.Collapsed;
        }

        DrawReferenceLine();
        UpdateScaleAndPlayfield();
    }

    private void ClearPoints_Click(object sender, RoutedEventArgs e)
    {
        _referencePointA = null;
        _referencePointB = null;
        OverlayCanvas.Children.Clear();

        PlayfieldRectangle.Visibility = Visibility.Collapsed;

        PointStatus.Text = MapImage.Source is null
            ? "No image loaded"
            : "Click reference point A";

        ScaleStatus.Text = "Scale: —";
        PlayfieldStatus.Text = "Playfield: —";
    }

    private void SettingsTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
    {
        if(_initialised)
            UpdateScaleAndPlayfield();
    }

    private void UpdateScaleAndPlayfield()
    {
        if (_referencePointA is null || _referencePointB is null)
        {
            ScaleStatus.Text = "Scale: —";
            PlayfieldStatus.Text = "Playfield: —";
            return;
        }

        if (!double.TryParse(
                DistanceTextBox.Text,
                NumberStyles.Float,
                CultureInfo.InvariantCulture,
                out var distanceMetres) ||
            distanceMetres <= 0)
        {
            ScaleStatus.Text = "Scale: enter a positive reference distance";
            PlayfieldStatus.Text = "Playfield: —";
            PlayfieldRectangle.Visibility = Visibility.Collapsed;
            return;
        }

        var dx = _referencePointB.Value.X - _referencePointA.Value.X;
        var dy = _referencePointB.Value.Y - _referencePointA.Value.Y;
        var pixelDistance = Math.Sqrt(dx * dx + dy * dy);

        if (pixelDistance <= 0.0001)
        {
            ScaleStatus.Text = "Scale: reference points must be different";
            PlayfieldStatus.Text = "Playfield: —";
            PlayfieldRectangle.Visibility = Visibility.Collapsed;
            return;
        }

        var metresPerPixel = distanceMetres / pixelDistance;
        var pixelsPerMetre = pixelDistance / distanceMetres;

        ScaleStatus.Text =
            $"Scale: {metresPerPixel:0.000000} m/pixel  " +
            $"({pixelsPerMetre:0.000000} px/m)";

        var widthMetres = OutputWidth * metresPerPixel;
        var heightMetres = OutputHeight * metresPerPixel;

        PlayfieldStatus.Text =
            $"Playfield: {widthMetres:0.0} × {heightMetres:0.0} m";

        CreatePlayfield();
    }

    private void CreatePlayfield()
    {
        if (_referencePointA is null || _referencePointB is null)
            return;

        if (!double.TryParse(
                DistanceTextBox.Text,
                NumberStyles.Float,
                CultureInfo.InvariantCulture,
                out var distanceMetres) ||
            distanceMetres <= 0)
            return;

        var dx = _referencePointB.Value.X - _referencePointA.Value.X;
        var dy = _referencePointB.Value.Y - _referencePointA.Value.Y;
        var pixelDistance = Math.Sqrt(dx * dx + dy * dy);

        if (pixelDistance <= 0.0001)
            return;

        var metresPerPixel = distanceMetres / pixelDistance;

        // At this stage the rectangle is represented at the map's native
        // pixel scale.  Its dimensions correspond to 1280x960 output pixels.
        var sourceWidth = OutputWidth;
        var sourceHeight = OutputHeight;

        // Centre the playfield initially, then clamp it inside the source image.
        var left = Math.Max(0, (_imageWidth - sourceWidth) / 2);
        var top = Math.Max(0, (_imageHeight - sourceHeight) / 2);

        if (_imageWidth < sourceWidth || _imageHeight < sourceHeight)
        {
            PlayfieldStatus.Text += "  (source image is smaller than 1280×960)";
        }

        Canvas.SetLeft(PlayfieldRectangle, left);
        Canvas.SetTop(PlayfieldRectangle, top);
        PlayfieldRectangle.Visibility = Visibility.Visible;

        // Keep the calculated physical size visible in the status bar.
        PlayfieldStatus.Text =
            $"Playfield: {OutputWidth:0}×{OutputHeight:0} px  " +
            $"= {OutputWidth * metresPerPixel:0.0}×{OutputHeight * metresPerPixel:0.0} m";
    }

    private void DrawReferenceLine()
    {
        OverlayCanvas.Children.Clear();

        if (_referencePointA is null)
            return;

        AddMarker(_referencePointA.Value, "A");

        if (_referencePointB is null)
            return;

        AddMarker(_referencePointB.Value, "B");

        var line = new Line
        {
            X1 = _referencePointA.Value.X,
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

        var text = new System.Windows.Controls.TextBlock
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

    private void MapImage_MouseMove(object sender, MouseEventArgs e)
    {
        if (!_draggingPlayfield)
            return;

        var current = e.GetPosition(MapWorkspace);

        var dx = current.X - _dragStartMouse.X;
        var dy = current.Y - _dragStartMouse.Y;

        var newLeft = _playfieldStartLeft + dx;
        var newTop = _playfieldStartTop + dy;

        newLeft = Math.Max(0, Math.Min(newLeft, Math.Max(0, _imageWidth - OutputWidth)));
        newTop = Math.Max(0, Math.Min(newTop, Math.Max(0, _imageHeight - OutputHeight)));

        Canvas.SetLeft(PlayfieldRectangle, newLeft);
        Canvas.SetTop(PlayfieldRectangle, newTop);

        UpdatePlayfieldPositionStatus(newLeft, newTop);
    }

    private void MapImage_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        if (_draggingPlayfield)
        {
            _draggingPlayfield = false;
            MapImage.ReleaseMouseCapture();
        }
    }

    private void MapScrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        // Deliberately unused in Stage 1.  Image zooming can be added later.
    }

    private void UpdatePlayfieldPositionStatus(double left, double top)
    {
        var widthText = $"{OutputWidth:0}";
        var heightText = $"{OutputHeight:0}";
        PlayfieldStatus.Text =
            $"Playfield: {widthText}×{heightText} px  " +
            $"at source ({left:0}, {top:0})";
    }
}
