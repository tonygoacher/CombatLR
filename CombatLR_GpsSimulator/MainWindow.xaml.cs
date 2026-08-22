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
                _bearingDeg = _map.BearingDegrees(_lastMotionPixel, p);
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
        HeadingLine.RenderTransform = new RotateTransform(_bearingDeg, 11, 11);

        PositionText.Text =
            $"Pixel: {_gpsPixel.X:F1}, {_gpsPixel.Y:F1}    " +
            $"GPS: {gps.Latitude:F7}, {gps.Longitude:F7}";
        MotionText.Text =
            $"Speed: {_speedMps * 3.6:F1} km/h    Bearing: {_bearingDeg:F0}°";
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
            $"{_speedMps * 1.94384449:F1},{_bearingDeg:F1},{date},,,A");

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
    public double RotationDegrees { get; }
    public double MetresPerPixel { get; }

    public double SourceCropX { get; }
    public double SourceCropY { get; }
    public GpsPoint TopLeft { get; }
    public GpsPoint TopRight { get; }
    public GpsPoint BottomLeft { get; }
    public GpsPoint BottomRight { get; }

    public Point ReferencePixelA { get; }
    public GpsPoint ReferenceGpsA { get; }

    public Point ReferencePixelB { get; }
    public GpsPoint ReferenceGpsB { get; }
    public GpsPoint Centre { get; }
    public double PixelsPerMetre { get; }
    private MapDefinition(
        string bmp,
        string json,
        int width,
        int height,
        double rotation,
        double mpp,
        double sourceCropX,
        double sourceCropY,
        GpsPoint tl,
        GpsPoint tr,
        GpsPoint bl,
        GpsPoint br,
        Point referencePixelA,
        GpsPoint referenceGpsA,
        Point referencePixelB,
        GpsPoint referenceGpsB)
    {
        BitmapPath = bmp;
        JsonPath = json;
        Width = width;
        Height = height;
        RotationDegrees = rotation;
        MetresPerPixel = mpp;

        TopLeft = tl;
        TopRight = tr;
        BottomLeft = bl;
        BottomRight = br;

        ReferencePixelA = referencePixelA;
        ReferenceGpsA = referenceGpsA;
        ReferencePixelB = referencePixelB;
        ReferenceGpsB = referenceGpsB;
        SourceCropX = sourceCropX;
        SourceCropY = sourceCropY;
    }

    public static MapDefinition FromJson(string bmp, string jsonPath, string text)
    {
        using var doc = JsonDocument.Parse(text);
        var r = doc.RootElement;
        var img = r.GetProperty("Image");
        var pf = r.GetProperty("Playfield");
        var crop = r.GetProperty("SourceCrop");
        var reference = r.GetProperty("Reference");
        var pointA = reference.GetProperty("PointA");
        var gpsA = reference.GetProperty("GpsA");
        var pointB = reference.GetProperty("PointB");
        var gpsB = reference.GetProperty("GpsB");


        static GpsPoint P(JsonElement e) =>
            new(e.GetProperty("Latitude").GetDouble(), e.GetProperty("Longitude").GetDouble());

        return new MapDefinition(
            bmp, jsonPath,
            img.GetProperty("Width").GetInt32(),
            img.GetProperty("Height").GetInt32(),
            r.GetProperty("RotationDegrees").GetDouble(),
            r.GetProperty("MetresPerPixel").GetDouble(),
            crop.GetProperty("X").GetDouble(),
            crop.GetProperty("Y").GetDouble(),
            P(pf.GetProperty("TopLeft")),
            P(pf.GetProperty("TopRight")),
            P(pf.GetProperty("BottomLeft")),
            P(pf.GetProperty("BottomRight")),
           new Point(
            pointA.GetProperty("X").GetDouble(),
            pointA.GetProperty("Y").GetDouble()),

        new GpsPoint(
            gpsA.GetProperty("Latitude").GetDouble(),
            gpsA.GetProperty("Longitude").GetDouble()),

        new Point(
            pointB.GetProperty("X").GetDouble(),
            pointB.GetProperty("Y").GetDouble()),

        new GpsPoint(
            gpsB.GetProperty("Latitude").GetDouble(),
            gpsB.GetProperty("Longitude").GetDouble()));

        new GpsPoint(
            gpsB.GetProperty("Latitude").GetDouble(),
            gpsB.GetProperty("Longitude").GetDouble());
    }

    public GpsPoint PixelToGps(Point p)
    {
        // map.bmp coordinates are relative to the exported crop.
        // MapBuilder's calibration points are in source-image coordinates.
        var imagePoint = new Point(
            p.X + SourceCropX,
            p.Y + SourceCropY);

        String p2gps=
    $"PixelToGps: map=({p.X:F3},{p.Y:F3}) " +
    $"crop=({SourceCropX:F3},{SourceCropY:F3}) " +
    $"source=({imagePoint.X:F3},{imagePoint.Y:F3})";

        p2gps = 
            $"PixelToGps: A=({ReferencePixelA.X:F3},{ReferencePixelA.Y:F3}) " +
            $"B=({ReferencePixelB.X:F3},{ReferencePixelB.Y:F3})";

        // This is the same image -> geographic transform used by
        // CombatlrMapBuilder.BuildImageToGeoTransform().
        var meanLat =
            ((ReferenceGpsA.Latitude + ReferenceGpsB.Latitude) / 2.0)
            * Math.PI / 180.0;

        const double earthRadius = 6371008.8;

        var dLon =
            (ReferenceGpsB.Longitude - ReferenceGpsA.Longitude)
            * Math.PI / 180.0;

        var dLat =
            (ReferenceGpsB.Latitude - ReferenceGpsA.Latitude)
            * Math.PI / 180.0;

        var east =
            dLon * Math.Cos(meanLat) * earthRadius;

        var north =
            dLat * earthRadius;

        var dx =
            ReferencePixelB.X - ReferencePixelA.X;

        var dy =
            ReferencePixelB.Y - ReferencePixelA.Y;

       p2gps = 
    $"PixelToGps AB: pixel=({dx:F3},{dy:F3}) " +
    $"geo=({east:F3},{north:F3})";

        var pixelDistance =
            Math.Sqrt(dx * dx + dy * dy);

        var geoDistance =
            Math.Sqrt(east * east + north * north);

        if (pixelDistance < 0.000001 || geoDistance < 0.000001)
            return ReferenceGpsA;



        var scale = geoDistance / pixelDistance;

        /*
         * Build the same GameCoordinate -> map-pixel transform
         * used by CombatLR's MapReference::Configure(), then
         * invert it for Pixel -> GameCoordinate.
         *
         * Image/map coordinates:
         *   X increases right
         *   Y increases down
         *
         * Game coordinates:
         *   East  increases east
         *   North increases north
         */

        var gameAlongEast =
            east / geoDistance;

        var gameAlongNorth =
            north / geoDistance;

        /*
         * Perpendicular GameCoordinate vector.
         */
        var gameAcrossEast =
            -gameAlongNorth;

        var gameAcrossNorth =
            gameAlongEast;

        /*
         * Unit vector along A -> B in map space.
         */
        var mapAlongX =
            dx / pixelDistance;

        var mapAlongY =
            dy / pixelDistance;

        /*
         * Perpendicular map vector.
         */
        var mapAcrossX =
            -mapAlongY;

        var mapAcrossY =
            mapAlongX;

        var pixelsPerMetre =
            pixelDistance / geoDistance;

        /*
         * Same transform as MapReference::Configure().
         *
         * GameCoordinate -> map pixel:
         *
         *   mapX = east  * eastToMapX
         *        + north * northToMapX
         *
         *   mapY = east  * eastToMapY
         *        + north * northToMapY
         */
        var eastToMapX =
            pixelsPerMetre *
            (mapAlongX * gameAlongEast +
             mapAcrossX * gameAcrossEast);

        var eastToMapY =
            pixelsPerMetre *
            (mapAlongY * gameAlongEast +
             mapAcrossY * gameAcrossEast);

        var northToMapX =
            pixelsPerMetre *
            (mapAlongX * gameAlongNorth +
             mapAcrossX * gameAcrossNorth);

        var northToMapY =
            pixelsPerMetre *
            (mapAlongY * gameAlongNorth +
             mapAcrossY * gameAcrossNorth);

        /*
         * Invert the 2x2 GameCoordinate -> map matrix.
         */
        var determinant =
            eastToMapX * northToMapY -
            northToMapX * eastToMapY;

        if (Math.Abs(determinant) < 0.000001)
            return ReferenceGpsA;

        var eastPerPixelX =
            northToMapY / determinant;

        var eastPerPixelY =
            -northToMapX / determinant;

        var northPerPixelX =
            -eastToMapY / determinant;

        var northPerPixelY =
            eastToMapX / determinant;

        p2gps = 
    $"PixelToGps transform: " +
    $"E/X={eastPerPixelX:F6} " +
    $"E/Y={eastPerPixelY:F6} " +
    $"N/X={northPerPixelX:F6} " +
    $"N/Y={northPerPixelY:F6}";
    


        var sourceDx =
            imagePoint.X - ReferencePixelA.X;

        var sourceDy =
            imagePoint.Y - ReferencePixelA.Y;

        var localEast =
            eastPerPixelX * sourceDx +
            eastPerPixelY * sourceDy;

        var localNorth =
            northPerPixelX * sourceDx +
            northPerPixelY * sourceDy;
        p2gps = 
    $"PixelToGps: sourceDelta=({sourceDx:F3},{sourceDy:F3}) " +
    $"local EN=({localEast:F3},{localNorth:F3})";


        var lat =
            ReferenceGpsA.Latitude +
            (localNorth / earthRadius) *
            180.0 / Math.PI;

        var lon =
            ReferenceGpsA.Longitude +
            (localEast /
             (earthRadius *
              Math.Cos(ReferenceGpsA.Latitude * Math.PI / 180.0))) *
            180.0 / Math.PI;

        return new GpsPoint(lat, lon);
    }

    public Point GpsToPixel(GpsPoint gps)
    {
        /*
         * Convert GPS position into local east/north metres relative
         * to ReferenceGpsA.
         */
        const double earthRadius = 6371008.8;

        var meanLat =
            ((ReferenceGpsA.Latitude + gps.Latitude) / 2.0)
            * Math.PI / 180.0;

        var dLat =
            (gps.Latitude - ReferenceGpsA.Latitude)
            * Math.PI / 180.0;

        var dLon =
            (gps.Longitude - ReferenceGpsA.Longitude)
            * Math.PI / 180.0;

        var north =
            dLat * earthRadius;

        var east =
            dLon * Math.Cos(meanLat) * earthRadius;

        /*
         * Reconstruct the same A->B geometry used by PixelToGps().
         */
        var dx =
            ReferencePixelB.X -
            ReferencePixelA.X;

        var dy =
            ReferencePixelB.Y -
            ReferencePixelA.Y;

        var pixelDistance =
            Math.Sqrt(dx * dx + dy * dy);

        /*
         * A->B geographic vector.
         */
        var refMeanLat =
            ((ReferenceGpsA.Latitude +
              ReferenceGpsB.Latitude) / 2.0)
            * Math.PI / 180.0;

        var refDLon =
            (ReferenceGpsB.Longitude -
             ReferenceGpsA.Longitude)
            * Math.PI / 180.0;

        var refDLat =
            (ReferenceGpsB.Latitude -
             ReferenceGpsA.Latitude)
            * Math.PI / 180.0;

        var refEast =
            refDLon *
            Math.Cos(refMeanLat) *
            earthRadius;

        var refNorth =
            refDLat *
            earthRadius;

        var geoDistance =
            Math.Sqrt(
                refEast * refEast +
                refNorth * refNorth);

        if (pixelDistance < 0.000001 ||
            geoDistance < 0.000001)
        {
            return ReferencePixelA;
        }

        /*
         * Same unit vectors as PixelToGps().
         */
        var gameAlongEast =
            refEast / geoDistance;

        var gameAlongNorth =
            refNorth / geoDistance;

        var gameAcrossEast =
            -gameAlongNorth;

        var gameAcrossNorth =
            gameAlongEast;

        var mapAlongX =
            dx / pixelDistance;

        var mapAlongY =
            dy / pixelDistance;

        var mapAcrossX =
            -mapAlongY;

        var mapAcrossY =
            mapAlongX;

        var pixelsPerMetre =
            pixelDistance / geoDistance;

        /*
         * Same GameCoordinate -> map-pixel matrix as
         * MapReference::Configure().
         */
        var eastToMapX =
            pixelsPerMetre *
            (mapAlongX * gameAlongEast +
             mapAcrossX * gameAcrossEast);

        var eastToMapY =
            pixelsPerMetre *
            (mapAlongY * gameAlongEast +
             mapAcrossY * gameAcrossEast);

        var northToMapX =
            pixelsPerMetre *
            (mapAlongX * gameAlongNorth +
             mapAcrossX * gameAcrossNorth);

        var northToMapY =
            pixelsPerMetre *
            (mapAlongY * gameAlongNorth +
             mapAcrossY * gameAcrossNorth);

        /*
         * Convert the local EN position to source-image pixels.
         *
         * ReferencePixelA/B are in source-image coordinates.
         */
        var sourceX =
            ReferencePixelA.X +
            east * eastToMapX +
            north * northToMapX;

        var sourceY =
            ReferencePixelA.Y +
            east * eastToMapY +
            north * northToMapY;

        /*
         * Convert source-image coordinates back to map.bmp
         * coordinates by removing the exported crop offset.
         */
        return new Point(
            sourceX - SourceCropX,
            sourceY - SourceCropY);
    }

    public double PixelDistanceMetres(Point a, Point b)
    {
        double dx = b.X - a.X, dy = b.Y - a.Y;
        return Math.Sqrt(dx * dx + dy * dy) * MetresPerPixel;
    }

    public double BearingDegrees(Point a, Point b)
    {
        // Image Y increases downwards. Convert image displacement to a map
        // north/east displacement using the map rotation.
        double dx = b.X - a.X;
        double dy = b.Y - a.Y;

        double theta = RotationDegrees * Math.PI / 180.0;
        double east = dx * Math.Cos(theta) - dy * Math.Sin(theta);
        double north = -(dx * Math.Sin(theta) + dy * Math.Cos(theta));

        double bearing = Math.Atan2(east, north) * 180.0 / Math.PI;
        return (bearing + 360.0) % 360.0;
    }
}

public readonly record struct GpsPoint(double Latitude, double Longitude);
