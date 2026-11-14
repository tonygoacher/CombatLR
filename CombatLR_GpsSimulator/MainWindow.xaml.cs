using System;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Windows.Controls;

using System.Diagnostics;

namespace CombatLRGpsSimulator;

public partial class MainWindow : Window
{
    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromMilliseconds(200) };
    private SerialPort? _serial;
    private MapDefinition? _map;
    private bool _dragging;
    private Point _gpsPixel;
    private Point _lastMotionPixel;
    private DateTime _lastPositionTime;
    private double _speedMps;
    private double _bearingDeg;

    private double _displayBearingDeg;

    private bool _hasDisplayBearing;

    public MainWindow()
    {
        InitializeComponent();
        RefreshPorts();
        _timer.Tick += (_, _) => EmitGps();
        System.IO.Path.Combine(AppContext.BaseDirectory, "map.bmp");
        var bmp = System.IO.Path.Combine(AppContext.BaseDirectory, "map.bmp");
        var json = System.IO.Path.Combine(AppContext.BaseDirectory, "map.json");
        LoadMap(bmp, json);
    }

    private void About_Click(object sender, RoutedEventArgs e)
    {
        var version = System.Reflection.Assembly
            .GetExecutingAssembly()
            .GetName()
            .Version?
            .ToString(3) ?? "DEV";

        MessageBox.Show(
            this,
            $"CombatLR GPS Simulator\nVersion {version}",
            "About CombatLR GPS Simulator",
            MessageBoxButton.OK,
            MessageBoxImage.Information);
    }

    private void RefreshPorts()
    {
        var selected = PortCombo.SelectedItem?.ToString();
        PortCombo.Items.Clear();
        foreach (var p in SerialPort.GetPortNames()) PortCombo.Items.Add(p);
        if (selected != null && PortCombo.Items.Contains(selected))
            PortCombo.SelectedItem = selected;
        else if (PortCombo.Items.Count > 0) PortCombo.SelectedIndex = 0;
    }

    private void RefreshPorts_Click(object sender, RoutedEventArgs e) => RefreshPorts();

    private void OpenMap_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "Bitmap (*.bmp)|*.bmp|All files (*.*)|*.*",
            Title = "Select CombatLR map.bmp"
        };
        if (dlg.ShowDialog() != true) return;

        var bmp = dlg.FileName;
        var json = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(bmp)!, "map.json");
        if (!File.Exists(json))
        {
            var jdlg = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "JSON (*.json)|*.json",
                FileName = "map.json",
                Title = "Select matching map.json"
            };
            if (jdlg.ShowDialog() != true) return;
            json = jdlg.FileName;
        }
        LoadMap(bmp, json);
    }

    private void Reload_Click(object sender, RoutedEventArgs e)
    {
        if (_map != null) LoadMap(_map.BitmapPath, _map.JsonPath);
    }

    private void LoadMap(string bmpPath, string jsonPath)
    {
        if (!File.Exists(bmpPath) || !File.Exists(jsonPath))
        {
            StatusText.Text = "Map files not found.";
            return;
        }

        try
        {
            _map = MapDefinition.FromJson(bmpPath, jsonPath, File.ReadAllText(jsonPath));

            var image = new BitmapImage();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.UriSource = new Uri(System.IO.Path.GetFullPath(bmpPath));
            image.EndInit();

            MapImage.Source = image;
            MapImage.Width = _map.Width;
            MapImage.Height = _map.Height;
          
           
          

            _gpsPixel = new Point(_map.Width / 2.0, _map.Height / 2.0);
            _lastMotionPixel = _gpsPixel;
            _lastPositionTime = DateTime.UtcNow;
            _speedMps = 0;
            UpdateMarker();

            StatusText.Text = $"Loaded {System.IO.Path.GetFileName(bmpPath)} + {System.IO.Path.GetFileName(jsonPath)}";
        }
        catch (Exception ex)
        {
            StatusText.Text = "Load error: " + ex.Message;
        }
    }

    private void MapHost_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (_map == null) return;
        _dragging = true;
        MapHost.CaptureMouse();
        MoveGps(e.GetPosition(MapImage));


    }

    private void MapHost_MouseMove(object sender, MouseEventArgs e)
    {
        if (!_dragging || _map == null) return;
        MoveGps(e.GetPosition(MapHost));
    }

    private void MapHost_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
    {
        _dragging = false;
        MapHost.ReleaseMouseCapture();
        _speedMps = 0;
        UpdateMarker();
    }

    private void MoveGps(Point p)
    {
        if (_map == null) return;

        StatusText.Text =
            $"Mouse: X={p.X:F1}, Y={p.Y:F1}    " +
            $"Map: {_map.Width:F0}x{_map.Height:F0}";

        p.X = Math.Clamp(p.X, 0, _map.Width);
        p.Y = Math.Clamp(p.Y, 0, _map.Height);

        var now = DateTime.UtcNow;
        if (_lastPositionTime != default)
        {
            var dt = Math.Max(0.001, (now - _lastPositionTime).TotalSeconds);
            var metres = _map.PixelDistanceMetres(_lastMotionPixel, p);
            _speedMps = metres / dt;
            if (metres > 0.001)
            {
                _bearingDeg =
                    _map.BearingDegrees(_lastMotionPixel, p);

                if (!_hasDisplayBearing)
                {
                    _displayBearingDeg = _bearingDeg;
                    _hasDisplayBearing = true;
                }
                else
                {
                    const double smoothing = 0.10;

                    double delta =
                        ((_bearingDeg - _displayBearingDeg + 540.0) % 360.0) -
                        180.0;

                    _displayBearingDeg +=
                        delta * smoothing;

                    _displayBearingDeg =
                        (_displayBearingDeg + 360.0) % 360.0;
                }
            }
        }

        _lastMotionPixel = p;
        _lastPositionTime = now;
        _gpsPixel = p;


        UpdateMarker();
    }

    private void UpdateMarker()
    {
        if (_map == null) return;
        var gps = _map.PixelToGps(_gpsPixel);

        Canvas.SetLeft(GpsMarker, _gpsPixel.X - 11);
        Canvas.SetTop(GpsMarker, _gpsPixel.Y - 11);
        Canvas.SetLeft(HeadingLine, _gpsPixel.X - 11);
        Canvas.SetTop(HeadingLine, _gpsPixel.Y - 11);

        HeadingLine.RenderTransform =
            new RotateTransform(
                _displayBearingDeg - _map.MapUpHeading,
                11,
                11);

        PositionText.Text =
            $"Pixel: {_gpsPixel.X:F1}, {_gpsPixel.Y:F1}    " +
            $"GPS: {gps.Latitude:F7}, {gps.Longitude:F7}";
        MotionText.Text =
            $"Speed: {_speedMps * 3.6:F1} km/h    Bearing: {_displayBearingDeg:F0}°";
    }

    private void Connect_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (_serial?.IsOpen == true)
            {
                _serial.Close();
                _serial.Dispose();
                _serial = null;
                ConnectButton.Content = "Connect";
                StatusText.Text = "Serial port disconnected.";
                return;
            }

            if (PortCombo.SelectedItem == null)
            {
                StatusText.Text = "Select a COM port first.";
                return;
            }

            int baud = int.Parse(((ComboBoxItem)BaudCombo.SelectedItem).Content.ToString()!);
            _serial = new SerialPort(PortCombo.SelectedItem.ToString()!, baud, Parity.None, 8, StopBits.One);
            _serial.Open();
            ConnectButton.Content = "Disconnect";
            StatusText.Text = $"Connected to {_serial.PortName} at {baud} baud.";
        }
        catch (Exception ex)
        {
            StatusText.Text = "Serial error: " + ex.Message;
        }
    }

    private void Start_Click(object sender, RoutedEventArgs e)
    {
        if (_serial?.IsOpen != true)
        {
            StatusText.Text = "Connect a COM port before starting GPS.";
            return;
        }
        _timer.Start();
        StatusText.Text = "GPS output running.";
    }

    private void Stop_Click(object sender, RoutedEventArgs e)
    {
        _timer.Stop();
        StatusText.Text = "GPS output stopped.";
    }

    private void ClearNmea_Click(object sender, RoutedEventArgs e) => NmeaList.Items.Clear();

    private void EmitGps()
    {
        if (_map == null || _serial?.IsOpen != true) return;

        var gps = _map.PixelToGps(_gpsPixel);
        var now = DateTime.UtcNow;

        string time = now.ToString("HHmmss.00", CultureInfo.InvariantCulture);
        string date = now.ToString("ddMMyy", CultureInfo.InvariantCulture);

        var rmc = BuildNmea(
            $"GPRMC,{time},A,{ToNmeaLat(gps.Latitude)},{LatHem(gps.Latitude)}," +
            $"{ToNmeaLon(gps.Longitude)},{LonHem(gps.Longitude)}," +
            $"{_speedMps * 1.94384449:F1},{_displayBearingDeg:F1},{date},,,A");

        var gga = BuildNmea(
            $"GPGGA,{time},{ToNmeaLat(gps.Latitude)},{LatHem(gps.Latitude)}," +
            $"{ToNmeaLon(gps.Longitude)},{LonHem(gps.Longitude)},1,08,1.0,100.0,M,0.0,M,,");

        try
        {
            _serial.WriteLine(rmc.TrimEnd('\r', '\n'));
            _serial.WriteLine(gga.TrimEnd('\r', '\n'));

            AddNmea(rmc);
            AddNmea(gga);
        }
        catch (Exception ex)
        {
            _timer.Stop();
            StatusText.Text = "Serial write error: " + ex.Message;
        }
    }

    private void AddNmea(string sentence)
    {
        NmeaList.Items.Add($"{DateTime.Now:HH:mm:ss.fff}  {sentence.Trim()}");
        while (NmeaList.Items.Count > 500)
            NmeaList.Items.RemoveAt(0);
        if (NmeaList.Items.Count > 0)
            NmeaList.ScrollIntoView(NmeaList.Items[NmeaList.Items.Count - 1]);
    }

    private static string BuildNmea(string body)
    {
        byte checksum = 0;
        foreach (char c in body) checksum ^= (byte)c;
        return $"${body}*{checksum:X2}\r\n";
    }

    private static string ToNmeaLat(double lat)
    {
        double a = Math.Abs(lat);
        int deg = (int)a;
        double minutes = (a - deg) * 60;
        return $"{deg:00}{minutes:00.0000}";
    }

    private static string ToNmeaLon(double lon)
    {
        double a = Math.Abs(lon);
        int deg = (int)a;
        double minutes = (a - deg) * 60;
        return $"{deg:000}{minutes:00.0000}";
    }

    private static string LatHem(double lat) => lat >= 0 ? "N" : "S";
    private static string LonHem(double lon) => lon >= 0 ? "E" : "W";

    protected override void OnClosed(EventArgs e)
    {
        _timer.Stop();
        if (_serial?.IsOpen == true) _serial.Close();
        _serial?.Dispose();
        base.OnClosed(e);
    }
}

public sealed class MapDefinition
{
    public string BitmapPath { get; }
    public string JsonPath { get; }

    public int Width { get; }
    public int Height { get; }

    public double MetresPerPixel { get; }

    public double SourceCropX { get; }
    public double SourceCropY { get; }

    public GpsPoint TopLeft { get; }
    public GpsPoint TopRight { get; }
    public GpsPoint BottomLeft { get; }
    public GpsPoint BottomRight { get; }

    public GpsPoint Centre { get; }

    public double MapUpHeading { get; }

    // Image/source pixel -> local EN metres
    //
    // East  = E0 + Ex * X + Ey * Y
    // North = N0 + Nx * X + Ny * Y
    //
    private double _eastOffset;
    private double _eastPerPixelX;
    private double _eastPerPixelY;

    private double _northOffset;
    private double _northPerPixelX;
    private double _northPerPixelY;

    private bool _transformValid;

    private const double EarthRadiusMetres = 6371008.8;

    private readonly CalibrationPoint[] _points;

    private MapDefinition(
        string bmp,
        string json,
        int width,
        int height,
        double metresPerPixel,
        double sourceCropX,
        double sourceCropY,
        GpsPoint topLeft,
        GpsPoint topRight,
        GpsPoint bottomLeft,
        GpsPoint bottomRight,
        GpsPoint centre,
        CalibrationPoint[] points)
    {
        BitmapPath = bmp;
        JsonPath = json;

        Width = width;
        Height = height;

        MetresPerPixel = metresPerPixel;

        SourceCropX = sourceCropX;
        SourceCropY = sourceCropY;

        TopLeft = topLeft;
        TopRight = topRight;
        BottomLeft = bottomLeft;
        BottomRight = bottomRight;

        Centre = centre;

        _points = points;

        BuildTransform();

        MapUpHeading = CalculateMapUpHeading();
    }

    public static MapDefinition FromJson(
        string bmp,
        string jsonPath,
        string text)
    {
        using var doc = JsonDocument.Parse(text);

        var root = doc.RootElement;

        var image =
            root.GetProperty("Image");

        var playfield =
            root.GetProperty("Playfield");

        var crop =
            root.GetProperty("SourceCrop");

        var reference =
            root.GetProperty("Reference");

        var referencePoints =
            reference.GetProperty("Points");

        var points =
            new List<CalibrationPoint>();

        foreach (var element in referencePoints.EnumerateArray())
        {
            var gps =
                element.GetProperty("Gps");

            points.Add(
                new CalibrationPoint(
                    element.GetProperty("Name").GetString() ?? "",
                    element.GetProperty("X").GetDouble(),
                    element.GetProperty("Y").GetDouble(),
                    gps.GetProperty("Latitude").GetDouble(),
                    gps.GetProperty("Longitude").GetDouble()));
        }

        if (points.Count < 3)
        {
            throw new InvalidDataException(
                "map.json must contain at least three calibration points.");
        }

        static GpsPoint ReadGps(JsonElement element)
        {
            return new GpsPoint(
                element.GetProperty("Latitude").GetDouble(),
                element.GetProperty("Longitude").GetDouble());
        }

        return new MapDefinition(
            bmp,
            jsonPath,

            image.GetProperty("Width").GetInt32(),
            image.GetProperty("Height").GetInt32(),

            root.GetProperty("MetresPerPixel").GetDouble(),

            crop.GetProperty("X").GetDouble(),
            crop.GetProperty("Y").GetDouble(),

            ReadGps(playfield.GetProperty("TopLeft")),
            ReadGps(playfield.GetProperty("TopRight")),
            ReadGps(playfield.GetProperty("BottomLeft")),
            ReadGps(playfield.GetProperty("BottomRight")),

            ReadGps(playfield.GetProperty("Centre")),

            points.ToArray());
    }

    private void BuildTransform()
    {
        _transformValid = false;

        var first = _points[0];

        var lat0 =
            first.Latitude *
            Math.PI / 180.0;

        var lon0 =
            first.Longitude *
            Math.PI / 180.0;

        /*
         * Build the same least-squares affine transform
         * used by CombatLR.mobile.
         *
         * East  = Ex * X + Ey * Y + E0
         * North = Nx * X + Ny * Y + N0
         */

        double sxx = 0;
        double sxy = 0;
        double sx = 0;

        double syy = 0;
        double sy = 0;

        double sn = 0;

        double sxe = 0;
        double sye = 0;
        double se = 0;

        double sxn = 0;
        double syn = 0;
        double snorth = 0;

        foreach (var point in _points)
        {
            var x = point.X;
            var y = point.Y;

            var lat =
                point.Latitude *
                Math.PI / 180.0;

            var lon =
                point.Longitude *
                Math.PI / 180.0;

            var east =
                (lon - lon0) *
                Math.Cos(lat0) *
                EarthRadiusMetres;

            var north =
                (lat - lat0) *
                EarthRadiusMetres;

            sxx += x * x;
            sxy += x * y;
            sx += x;

            syy += y * y;
            sy += y;

            sn += 1.0;

            sxe += x * east;
            sye += y * east;
            se += east;

            sxn += x * north;
            syn += y * north;
            snorth += north;
        }

        var matrix =
            new double[,]
            {
                { sxx, sxy, sx },
                { sxy, syy, sy },
                { sx,  sy,  sn }
            };

        var eastResult =
            Solve3x3(
                matrix,
                new[]
                {
                    sxe,
                    sye,
                    se
                });

        var northResult =
            Solve3x3(
                matrix,
                new[]
                {
                    sxn,
                    syn,
                    snorth
                });

        if (eastResult is null ||
            northResult is null)
        {
            return;
        }

        _eastPerPixelX = eastResult[0];
        _eastPerPixelY = eastResult[1];
        _eastOffset = eastResult[2];

        _northPerPixelX = northResult[0];
        _northPerPixelY = northResult[1];
        _northOffset = northResult[2];

        _transformValid = true;
    }

    private static double[]? Solve3x3(
        double[,] input,
        double[] rhs)
    {
        var a =
            new double[3, 4];

        for (var row = 0; row < 3; row++)
        {
            for (var col = 0; col < 3; col++)
                a[row, col] = input[row, col];

            a[row, 3] = rhs[row];
        }

        for (var column = 0; column < 3; column++)
        {
            var pivot = column;

            for (var row = column + 1;
                 row < 3;
                 row++)
            {
                if (Math.Abs(a[row, column]) >
                    Math.Abs(a[pivot, column]))
                {
                    pivot = row;
                }
            }

            if (Math.Abs(a[pivot, column]) < 1e-12)
                return null;

            if (pivot != column)
            {
                for (var col = column;
                     col < 4;
                     col++)
                {
                    var temp =
                        a[column, col];

                    a[column, col] =
                        a[pivot, col];

                    a[pivot, col] =
                        temp;
                }
            }

            var divisor =
                a[column, column];

            for (var col = column;
                 col < 4;
                 col++)
            {
                a[column, col] /= divisor;
            }

            for (var row = 0;
                 row < 3;
                 row++)
            {
                if (row == column)
                    continue;

                var factor =
                    a[row, column];

                for (var col = column;
                     col < 4;
                     col++)
                {
                    a[row, col] -=
                        factor *
                        a[column, col];
                }
            }
        }

        return new[]
        {
            a[0, 3],
            a[1, 3],
            a[2, 3]
        };
    }

    public GpsPoint PixelToGps(Point mapPixel)
    {
        if (!_transformValid)
            return Centre;

        /*
         * map.bmp coordinates are relative to the exported crop.
         *
         * Calibration points are in source-image coordinates.
         */

        var sourceX =
            mapPixel.X +
            SourceCropX;

        var sourceY =
            mapPixel.Y +
            SourceCropY;

        var east =
            _eastOffset +
            _eastPerPixelX * sourceX +
            _eastPerPixelY * sourceY;

        var north =
            _northOffset +
            _northPerPixelX * sourceX +
            _northPerPixelY * sourceY;

        /*
         * The affine coefficients are relative to
         * the GPS coordinate of calibration point A.
         */

        var origin =
            _points[0];

        var lat0 =
            origin.Latitude *
            Math.PI / 180.0;

        var lon0 =
            origin.Longitude *
            Math.PI / 180.0;

        var latitude =
            lat0 +
            north / EarthRadiusMetres;

        var longitude =
            lon0 +
            east /
            (EarthRadiusMetres *
             Math.Cos(lat0));

        return new GpsPoint(
            latitude * 180.0 / Math.PI,
            longitude * 180.0 / Math.PI);
    }

    public Point GpsToPixel(GpsPoint gps)
    {
        if (!_transformValid)
            return new Point(
                Width / 2.0,
                Height / 2.0);

        var origin =
            _points[0];

        var lat0 =
            origin.Latitude *
            Math.PI / 180.0;

        var lon0 =
            origin.Longitude *
            Math.PI / 180.0;

        var latitude =
            gps.Latitude *
            Math.PI / 180.0;

        var longitude =
            gps.Longitude *
            Math.PI / 180.0;

        var east =
            (longitude - lon0) *
            Math.Cos(lat0) *
            EarthRadiusMetres;

        var north =
            (latitude - lat0) *
            EarthRadiusMetres;

        /*
         * Invert:
         *
         * East  = Ex*X + Ey*Y + E0
         * North = Nx*X + Ny*Y + N0
         */

        var eastValue =
            east - _eastOffset;

        var northValue =
            north - _northOffset;

        var determinant =
            _eastPerPixelX *
            _northPerPixelY -
            _eastPerPixelY *
            _northPerPixelX;

        if (Math.Abs(determinant) < 1e-12)
        {
            return new Point(
                Width / 2.0,
                Height / 2.0);
        }

        var sourceX =
            (eastValue *
             _northPerPixelY -
             _eastPerPixelY *
             northValue) /
            determinant;

        var sourceY =
            (_eastPerPixelX *
             northValue -
             eastValue *
             _northPerPixelX) /
            determinant;

        return new Point(
            sourceX - SourceCropX,
            sourceY - SourceCropY);
    }

    public double PixelDistanceMetres(
        Point a,
        Point b)
    {
        var dx =
            b.X - a.X;

        var dy =
            b.Y - a.Y;

        /*
         * Use the affine transform rather than the
         * single MetresPerPixel value. This correctly
         * handles the map's rotation/shear.
         */

        var east =
            _eastPerPixelX * dx +
            _eastPerPixelY * dy;

        var north =
            _northPerPixelX * dx +
            _northPerPixelY * dy;

        return Math.Sqrt(
            east * east +
            north * north);
    }

    public double BearingDegrees(
        Point a,
        Point b)
    {
        var dx =
            b.X - a.X;

        var dy =
            b.Y - a.Y;

        if (Math.Abs(dx) < 0.000001 &&
            Math.Abs(dy) < 0.000001)
        {
            return 0.0;
        }

        /*
         * Convert the pixel movement directly into
         * local east/north movement.
         */

        var east =
            _eastPerPixelX * dx +
            _eastPerPixelY * dy;

        var north =
            _northPerPixelX * dx +
            _northPerPixelY * dy;

        var bearing =
            Math.Atan2(east, north) *
            180.0 /
            Math.PI;

        return (bearing + 360.0) % 360.0;
    }

    private double CalculateMapUpHeading()
    {
        if (!_transformValid)
            return 0.0;

        /*
         * Bitmap up is pixel movement (0,-1).
         * Convert that to EN and calculate its
         * compass bearing.
         */

        var east =
            -_eastPerPixelY;

        var north =
            -_northPerPixelY;

        var heading =
            Math.Atan2(east, north) *
            180.0 /
            Math.PI;

        return (heading + 360.0) % 360.0;
    }

    private sealed record CalibrationPoint(
        string Name,
        double X,
        double Y,
        double Latitude,
        double Longitude);
}

public readonly record struct GpsPoint(
    double Latitude,
    double Longitude);