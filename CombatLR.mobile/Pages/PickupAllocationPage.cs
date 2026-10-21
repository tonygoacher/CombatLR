using CombatLR.mobile.Models;

namespace CombatLR.mobile.Pages;

public partial class GamePlanEditorPage
{
    private async void EditPickupAllocation_Clicked(object? sender, EventArgs e)
    {
        if (_gamePlan.ZonePercentages.Count == 0)
        {
            _gamePlan.ZonePercentages.Add(new ZonePickupPercentage { Zone = 0 });
        }

        await Navigation.PushModalAsync(new PickupAllocationPage(_gamePlan));
    }
}

internal sealed class PickupAllocationPage : ContentPage
{
    private sealed class ThresholdRow
    {
        public required Entry Zone { get; init; }
        public required Entry Allocation { get; init; }
        public required Entry Health { get; init; }
        public required Entry Ammo { get; init; }
        public required Entry Weapon { get; init; }
        public required View View { get; init; }
    }

    private readonly GamePlan _plan;
    private readonly VerticalStackLayout _rowsLayout = new() { Spacing = 12 };
    private readonly List<ThresholdRow> _rows = new();

    public PickupAllocationPage(GamePlan plan)
    {
        _plan = plan;
        Title = "Pickup Allocation";
        BackgroundColor = Colors.Black;

        var heading = new Label
        {
            Text = "PICKUP ALLOCATION BY ZONE",
            FontSize = 18,
            FontAttributes = FontAttributes.Bold,
            TextColor = Colors.White
        };

        var explanation = new Label
        {
            Text = "Select % controls how many candidate pickups are retained. Type percentages control the mix of Auto pickups. Each row applies from its zone until the next threshold.",
            FontSize = 13,
            TextColor = Colors.LightGray
        };

        foreach (var item in _plan.ZonePercentages.OrderBy(p => p.Zone))
            AddRow(item);

        var addButton = new Button { Text = "Add zone threshold" };
        addButton.Clicked += (_, _) =>
        {
            var nextZone = _rows
                .Select(r => ParseInt(r.Zone.Text, 0))
                .DefaultIfEmpty(-1)
                .Max() + 1;
            AddRow(new ZonePickupPercentage { Zone = nextZone });
        };

        var saveButton = new Button { Text = "Save", BackgroundColor = Colors.DarkGreen };
        saveButton.Clicked += SaveClicked;

        var cancelButton = new Button { Text = "Cancel" };
        cancelButton.Clicked += async (_, _) => await Navigation.PopModalAsync();

        var actions = new Grid
        {
            ColumnDefinitions = new ColumnDefinitionCollection
            {
                new ColumnDefinition(GridLength.Star),
                new ColumnDefinition(GridLength.Star)
            },
            ColumnSpacing = 8
        };
        actions.Add(saveButton, 0, 0);
        actions.Add(cancelButton, 1, 0);

        Content = new ScrollView
        {
            Content = new VerticalStackLayout
            {
                Padding = 16,
                Spacing = 12,
                Children =
                {
                    heading,
                    explanation,
                    CreateColumnHeadings(),
                    _rowsLayout,
                    addButton,
                    actions
                }
            }
        };
    }

    private static View CreateColumnHeadings()
    {
        var grid = CreateGrid();
        string[] labels = { "Zone", "Select %", "Health", "Ammo", "Weapon" };
        for (var i = 0; i < labels.Length; i++)
        {
            grid.Add(new Label
            {
                Text = labels[i],
                FontSize = 10,
                TextColor = Colors.LightGray,
                HorizontalTextAlignment = TextAlignment.Center
            }, i, 0);
        }

        return grid;
    }

    private static Grid CreateGrid()
    {
        var grid = new Grid
        {
            ColumnDefinitions = new ColumnDefinitionCollection
            {
                new ColumnDefinition(new GridLength(0.8, GridUnitType.Star)),
                new ColumnDefinition(new GridLength(1.1, GridUnitType.Star)),
                new ColumnDefinition(new GridLength(1, GridUnitType.Star)),
                new ColumnDefinition(new GridLength(1, GridUnitType.Star)),
                new ColumnDefinition(new GridLength(1, GridUnitType.Star))
            },
            ColumnSpacing = 4
        };
        return grid;
    }

    private void AddRow(ZonePickupPercentage item)
    {
        var grid = CreateGrid();
        Entry MakeEntry(int value, string placeholder) => new()
        {
            Text = value.ToString(),
            Placeholder = placeholder,
            Keyboard = Keyboard.Numeric,
            FontSize = 13,
            HorizontalTextAlignment = TextAlignment.Center,
            TextColor = Colors.White,
            BackgroundColor = Color.FromArgb("#333333"),
            Margin = 0
        };

        var zone = MakeEntry(item.Zone, "0");
        var allocation = MakeEntry(item.AllocationPercentage, "40");
        var health = MakeEntry(item.HealthPercentage, "40");
        var ammo = MakeEntry(item.AmmoPercentage, "40");
        var weapon = MakeEntry(item.WeaponPercentage, "20");

        grid.Add(zone, 0, 0);
        grid.Add(allocation, 1, 0);
        grid.Add(health, 2, 0);
        grid.Add(ammo, 3, 0);
        grid.Add(weapon, 4, 0);

        var remove = new Button
        {
            Text = "Remove threshold",
            FontSize = 12,
            Padding = new Thickness(8, 3),
            HorizontalOptions = LayoutOptions.End
        };
        var row = new VerticalStackLayout { Spacing = 4, Children = { grid, remove } };
        remove.Clicked += (_, _) =>
        {
            if (_rows.Count <= 1)
                return;

            _rows.RemoveAll(r => ReferenceEquals(r.View, row));
            _rowsLayout.Children.Remove(row);
        };

        _rows.Add(new ThresholdRow
        {
            Zone = zone,
            Allocation = allocation,
            Health = health,
            Ammo = ammo,
            Weapon = weapon,
            View = row
        });
        _rowsLayout.Children.Add(row);
    }

    private async void SaveClicked(object? sender, EventArgs e)
    {
        var updated = new List<ZonePickupPercentage>();

        foreach (var row in _rows)
        {
            if (!TryRead(row.Zone.Text, out var zone) ||
                !TryRead(row.Allocation.Text, out var allocation) ||
                !TryRead(row.Health.Text, out var health) ||
                !TryRead(row.Ammo.Text, out var ammo) ||
                !TryRead(row.Weapon.Text, out var weapon) ||
                zone < 0)
            {
                await DisplayAlert("Invalid values", "Enter whole numbers. Zone IDs must be zero or greater.", "OK");
                return;
            }

            updated.Add(new ZonePickupPercentage
            {
                Zone = zone,
                AllocationPercentage = Math.Clamp(allocation, 0, 100),
                HealthPercentage = Math.Clamp(health, 0, 100),
                AmmoPercentage = Math.Clamp(ammo, 0, 100),
                WeaponPercentage = Math.Clamp(weapon, 0, 100)
            });
        }

        if (updated.Select(p => p.Zone).Distinct().Count() != updated.Count)
        {
            await DisplayAlert("Duplicate zone", "Each zone threshold must be unique.", "OK");
            return;
        }

        if (!updated.Any(p => p.Zone == 0))
        {
            await DisplayAlert("Zone 0 required", "Keep a threshold for zone 0; it provides the default settings.", "OK");
            return;
        }

        _plan.ZonePercentages = updated.OrderBy(p => p.Zone).ToList();
        await Navigation.PopModalAsync();
    }

    private static int ParseInt(string? text, int fallback) =>
        int.TryParse(text, out var value) ? value : fallback;

    private static bool TryRead(string? text, out int value) =>
        int.TryParse(text, out value);
}
