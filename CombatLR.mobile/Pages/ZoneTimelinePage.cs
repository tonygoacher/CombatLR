using CombatLR.mobile.Models;
using Microsoft.Maui.Graphics;

namespace CombatLR.mobile.Pages;

/// <summary>
/// Read-only Gantt view of the temporal zone plan. Select a zone to edit its
/// start time and lifespan in the existing ZoneEditPage.
/// </summary>
public sealed class ZoneTimelinePage : ContentPage
{
    private const float LeftMargin = 88f;
    private const float RightMargin = 28f;
    private const float TopMargin = 42f;
    private const float RowHeight = 54f;
    private const float BarHeight = 30f;
    private const float AxisHeight = 44f;
    private const float BasePixelsPerMinute = 12f;
    private const float MinimumZoom = 0.01f;
    private const float MaximumZoom = 4f;
    private const int EffectivelyUnboundedLifespanSeconds = 1_000_000;

    private readonly GamePlan _gamePlan;
    private readonly TimelineDrawable _drawable;
    private readonly GraphicsView _graphicsView;
    private readonly ScrollView _horizontalScroll;
    private readonly Label _zoomLabel;
    private float _zoom = 1f;

    public ZoneTimelinePage(GamePlan gamePlan)
    {
        _gamePlan = gamePlan ?? throw new ArgumentNullException(nameof(gamePlan));
        _drawable = new TimelineDrawable(_gamePlan, () => PixelsPerMinute);

        _graphicsView = new GraphicsView
        {
            Drawable = _drawable,
            HeightRequest = TopMargin + Math.Max(1, _gamePlan.Zones.Count) * RowHeight + AxisHeight + 12,
            BackgroundColor = Color.FromArgb("#101820"),
            HorizontalOptions = LayoutOptions.Start,
            VerticalOptions = LayoutOptions.Start
        };
        var tapGesture = new TapGestureRecognizer();
        tapGesture.Tapped += OnTimelineTapped;
        _graphicsView.GestureRecognizers.Add(tapGesture);

        _horizontalScroll = new ScrollView
        {
            Orientation = ScrollOrientation.Horizontal,
            Content = _graphicsView,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Always,
            VerticalScrollBarVisibility = ScrollBarVisibility.Never
        };

        _zoomLabel = new Label
        {
            Text = "100%",
            TextColor = Colors.White,
            FontSize = 13,
            VerticalTextAlignment = TextAlignment.Center,
            HorizontalTextAlignment = TextAlignment.Center,
            WidthRequest = 56
        };

        var zoomOut = new Button { Text = "−", FontSize = 22, WidthRequest = 44, HeightRequest = 40, Padding = 0 };
        var zoomIn = new Button { Text = "+", FontSize = 22, WidthRequest = 44, HeightRequest = 40, Padding = 0 };
        zoomOut.Clicked += (_, _) => SetZoom(_zoom / 1.5f);
        zoomIn.Clicked += (_, _) => SetZoom(_zoom * 1.5f);

        var heading = new Label
        {
            Text = "Zone Timeline",
            FontSize = 22,
            FontAttributes = FontAttributes.Bold,
            TextColor = Colors.White,
            VerticalTextAlignment = TextAlignment.Center
        };
        var toolbar = new HorizontalStackLayout
        {
            Spacing = 6,
            VerticalOptions = LayoutOptions.Center,
            Children = { zoomOut, _zoomLabel, zoomIn }
        };
        var header = new Grid
        {
            ColumnDefinitions = { new ColumnDefinition(GridLength.Star), new ColumnDefinition(GridLength.Auto) },
            Children = { heading, toolbar }
        };
        Grid.SetColumn(toolbar, 1);

        var hint = new Label
        {
            Text = "Select a blue zone or yellow transition to edit that zone. Use + / − to zoom; swipe horizontally to scroll.",
            FontSize = 13,
            TextColor = Color.FromArgb("#D0D8E0")
        };
        var legend = new HorizontalStackLayout
        {
            Spacing = 16,
            Children = { LegendItem("#2878D0", "Zone active period"), LegendItem("#F2C94C", "Transition overlap") }
        };
        var done = new Button { Text = "Done", HorizontalOptions = LayoutOptions.End, MinimumWidthRequest = 90 };
        done.Clicked += async (_, _) => await Navigation.PopModalAsync();

        var layout = new Grid
        {
            Padding = new Thickness(12),
            RowDefinitions =
            {
                new RowDefinition(GridLength.Auto),
                new RowDefinition(GridLength.Auto),
                new RowDefinition(GridLength.Star),
                new RowDefinition(GridLength.Auto),
                new RowDefinition(GridLength.Auto)
            },
            RowSpacing = 8,
            BackgroundColor = Color.FromArgb("#101820")
        };
        layout.Add(header, 0, 0);
        layout.Add(hint, 0, 1);
        layout.Add(_horizontalScroll, 0, 2);
        layout.Add(legend, 0, 3);
        layout.Add(done, 0, 4);

        Content = layout;
        Title = "Zone Timeline";
        BackgroundColor = Color.FromArgb("#101820");
        UpdateCanvasSize();
    }

    private float PixelsPerMinute => BasePixelsPerMinute * _zoom;

    private static View LegendItem(string hex, string label) => new HorizontalStackLayout
    {
        Spacing = 5,
        Children =
        {
            new BoxView { Color = Color.FromArgb(hex), WidthRequest = 14, HeightRequest = 14, VerticalOptions = LayoutOptions.Center },
            new Label { Text = label, TextColor = Colors.White, FontSize = 12, VerticalTextAlignment = TextAlignment.Center }
        }
    };

    private void SetZoom(float zoom)
    {
        var oldZoom = _zoom;
        var oldOffset = _horizontalScroll.ScrollX;
        _zoom = Math.Clamp(zoom, MinimumZoom, MaximumZoom);
        _zoomLabel.Text = $"{_zoom * 100:0}%";
        UpdateCanvasSize();

        // Keep the same approximate time under the left edge after zooming.
        MainThread.BeginInvokeOnMainThread(async () =>
        {
            await Task.Yield();
            var scaledOffset = oldZoom > 0 ? oldOffset * (_zoom / oldZoom) : 0;
            var maxOffset = Math.Max(0, _graphicsView.Width - _horizontalScroll.Width);
            await _horizontalScroll.ScrollToAsync(Math.Clamp(scaledOffset, 0, maxOffset), 0, false);
        });
    }

    private void UpdateCanvasSize()
    {
        var endMinute = GetTimelineEndMinute(_gamePlan.Zones);
        _graphicsView.WidthRequest = Math.Max(760, LeftMargin + endMinute * PixelsPerMinute + RightMargin);
        _graphicsView.HeightRequest = TopMargin + Math.Max(1, _gamePlan.Zones.Count) * RowHeight + AxisHeight + 12;
        _drawable.Invalidate();
        _graphicsView.Invalidate();
    }

    private static int GetTimelineEndMinute(List<GameZone> zones)
    {
        if (zones.Count == 0)
            return 60;

        var latestStart = zones.Max(z => Math.Max(0, z.StartTime / 60));
        var finiteEnds = zones
            .Where(z => z.LifespanSeconds > 0 && z.LifespanSeconds < EffectivelyUnboundedLifespanSeconds)
            .Select(z => (long)Math.Max(0, z.StartTime) + z.LifespanSeconds)
            .ToList();
        var endSeconds = Math.Max((long)(latestStart + 30) * 60,
            finiteEnds.Count > 0 ? finiteEnds.Max() : (long)(latestStart + 30) * 60);
        // A very large lifespan is the existing sentinel for a practically permanent final zone.
        return (int)Math.Clamp((endSeconds + 59) / 60, 30L, 1440L);
    }

    private async void OnTimelineTapped(object? sender, TappedEventArgs e)
    {
        if (_gamePlan.Zones.Count == 0)
            return;

        // GetPosition is relative to the GraphicsView's content coordinates, so
        // hit testing does not need to add the horizontal ScrollView offset.
        var point = e.GetPosition(_graphicsView);
        if (point is null)
            return;
        var zoneIndex = _drawable.HitTest(
            (float)point.Value.X,
            (float)point.Value.Y,
            (float)_graphicsView.Width,
            (float)_graphicsView.Height);
        if (zoneIndex < 0 || zoneIndex >= _gamePlan.Zones.Count)
            return;

        var zone = _gamePlan.Zones[zoneIndex];
        var editPage = new ZoneEditPage(zone, zoneIndex + 1);
        await Navigation.PushModalAsync(editPage);
        var changed = await editPage.Completion;
        if (!changed)
            return;

        // Zone zero is the timeline origin; preserve the established game-plan rule.
        if (_gamePlan.Zones.Count > 0)
            _gamePlan.Zones[0].StartTime = 0;

        UpdateCanvasSize();
    }

    private sealed class TimelineDrawable : IDrawable
    {
        private readonly GamePlan _plan;
        private readonly Func<float> _pixelsPerMinute;
        private readonly List<HitRegion> _hitRegions = new();
        private readonly record struct HitRegion(int ZoneIndex, RectF Bounds);

        public TimelineDrawable(GamePlan plan, Func<float> pixelsPerMinute)
        {
            _plan = plan;
            _pixelsPerMinute = pixelsPerMinute;
        }

        public void Invalidate() { }

        public int HitTest(float x, float y, float width, float height)
        {
            for (var i = _hitRegions.Count - 1; i >= 0; i--)
            {
                if (_hitRegions[i].Bounds.Contains(x, y))
                    return _hitRegions[i].ZoneIndex;
            }
            return -1;
        }

        public void Draw(ICanvas canvas, RectF dirtyRect)
        {
            _hitRegions.Clear();
            canvas.FillColor = Color.FromArgb("#101820");
            canvas.FillRectangle(dirtyRect);

            var zones = _plan.Zones;
            if (zones.Count == 0)
            {
                canvas.FontColor = Colors.White;
                canvas.FontSize = 16;
                canvas.DrawString("No zones configured", 16, 20, 260, 28, HorizontalAlignment.Left, VerticalAlignment.Center);
                return;
            }

            var ppm = _pixelsPerMinute();
            var endMinute = GetTimelineEndMinute(zones);
            var axisY = TopMargin + zones.Count * RowHeight + 8;
            var axisStartX = LeftMargin;
            var axisEndX = axisStartX + endMinute * ppm;

            canvas.FontColor = Color.FromArgb("#D0D8E0");
            canvas.FontSize = 11;
            canvas.DrawString("Elapsed time", LeftMargin, 4, 160, 24, HorizontalAlignment.Left, VerticalAlignment.Center);

            // Shared vertical grid/tick marks, labelled in whole minutes.
            var tickStep = ppm < 5 ? 10 : ppm < 12 ? 5 : 1;
            for (var minute = 0; minute <= endMinute; minute += tickStep)
            {
                var x = axisStartX + minute * ppm;
                canvas.StrokeColor = Color.FromArgb("#263542");
                canvas.StrokeSize = 1;
                canvas.DrawLine(x, TopMargin - 6, x, axisY);
                canvas.FontColor = Color.FromArgb("#D0D8E0");
                canvas.FontSize = 10;
                canvas.DrawString($"{minute}m", x - 22, axisY + 4, 44, 20, HorizontalAlignment.Center, VerticalAlignment.Center);
            }

            for (var i = 0; i < zones.Count; i++)
            {
                var zone = zones[i];
                var y = TopMargin + i * RowHeight;
                var startMinute = Math.Max(0, zone.StartTime / 60.0);
                var lifespan = Math.Max(0, zone.LifespanSeconds);
                var expiryMinute = startMinute + lifespan / 60.0;
                var hasTransition = i + 1 < zones.Count && lifespan > 0 && lifespan < EffectivelyUnboundedLifespanSeconds;
                var transitionStart = expiryMinute;
                var transitionEnd = expiryMinute;
                var transitionDuration = 0.0;
                if (hasTransition)
                {
                    // The transition is the overlap between the next zone starting
                    // and this zone expiring: [next start, previous expiry].
                    transitionStart = Math.Max(0, zones[i + 1].StartTime / 60.0);
                    transitionEnd = expiryMinute;
                    transitionDuration = transitionEnd - transitionStart;
                    hasTransition = transitionDuration > 0.0;
                }

                canvas.FontColor = Colors.White;
                canvas.FontSize = 13;
                canvas.DrawString($"Zone {i}", 8, y, LeftMargin - 14, BarHeight, HorizontalAlignment.Left, VerticalAlignment.Center);
                canvas.FillColor = Color.FromArgb("#263542");
                canvas.FillRoundedRectangle(axisStartX, y, endMinute * ppm, BarHeight, 4);

                var blueEnd = hasTransition ? transitionStart : Math.Min(expiryMinute, endMinute);
                DrawSegment(canvas, i, new RectF(axisStartX + (float)startMinute * ppm, y,
                    Math.Max(0, (float)(Math.Min(blueEnd, endMinute) - startMinute) * ppm), BarHeight),
                    "#2878D0", $"{Math.Max(0, Math.Round((blueEnd - startMinute), MidpointRounding.AwayFromZero)):0}m", true);

                if (hasTransition)
                {
                    var visibleStart = Math.Max(0, transitionStart);
                    var visibleEnd = Math.Min(endMinute, transitionEnd);
                    var transitionRect = new RectF(axisStartX + (float)visibleStart * ppm, y,
                        Math.Max(0, (float)(visibleEnd - visibleStart) * ppm), BarHeight);
                    DrawSegment(canvas, i, transitionRect, "#F2C94C", $"{Math.Round(transitionDuration):0}m", true);
                }

                // Draw the same yellow transition interval on the next zone's row.
                // The interval is intentionally horizontally aligned across both rows.
                if (i > 0)
                {
                    var previous = zones[i - 1];
                    var previousLifespan = Math.Max(0, previous.LifespanSeconds);
                    if (previousLifespan > 0 && previousLifespan < EffectivelyUnboundedLifespanSeconds)
                    {
                        var previousExpiry = Math.Max(0, previous.StartTime / 60.0) + previousLifespan / 60.0;
                        var incomingEnd = Math.Max(0, zone.StartTime / 60.0);
                        var incomingDuration = previousExpiry - incomingEnd;
                        if (incomingDuration > 0)
                        {
                            // Mirror the same transition interval on this row so
                            // the yellow overlap is aligned across both zones.
                            var visibleStart = Math.Max(0, incomingEnd);
                            var visibleEnd = Math.Min(endMinute, previousExpiry);
                            var incomingRect = new RectF(axisStartX + (float)visibleStart * ppm, y,
                                Math.Max(0, (float)(visibleEnd - visibleStart) * ppm), BarHeight);
                            DrawSegment(canvas, i, incomingRect, "#F2C94C", $"{Math.Round(incomingDuration):0}m", true);
                        }
                    }
                }

            }

            canvas.StrokeColor = Color.FromArgb("#D0D8E0");
            canvas.StrokeSize = 1;
            canvas.DrawLine(axisStartX, axisY, axisEndX, axisY);
        }

        private void DrawSegment(ICanvas canvas, int zoneIndex, RectF rect, string colour, string label, bool labelWhenFits)
        {
            if (rect.Width <= 0)
                return;
            canvas.FillColor = Color.FromArgb(colour);
            canvas.FillRectangle(rect);
            _hitRegions.Add(new HitRegion(zoneIndex, new RectF(rect.X, rect.Y - 4, rect.Width, rect.Height + 8)));
            if (labelWhenFits && rect.Width >= 26)
            {
                canvas.FontColor = colour == "#F2C94C" ? Color.FromArgb("#463600") : Colors.White;
                canvas.FontSize = 10;
                canvas.DrawString(label, rect.X, rect.Y, rect.Width, rect.Height,
                    HorizontalAlignment.Center, VerticalAlignment.Center);
            }
        }
    }
}
