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
        public required int AcceptedZoneId { get; set; }
        public required Entry Allocation { get; init; }
        public required Entry Health { get; init; }
        public required Entry Ammo { get; init; }
        public required Entry Weapon { get; init; }
        public required View View { get; init; }
    }

    private readonly GamePlan _plan;
    private readonly VerticalStackLayout _rowsLayout = new() { Spacing = 12 };
    private readonly List<ThresholdRow> _rows = new();
    private Button? _addButton;

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

        _addButton = new Button { Text = "Add zone threshold" };
        UpdateAddButtonState(_addButton);
        _addButton.Clicked += (_, _) =>
        {
            var usedZones = _rows.Select(r => r.AcceptedZoneId).ToHashSet();
            var nextZone = Enumerable.Range(0, _plan.Zones.Count)
                .FirstOrDefault(zoneId => !usedZones.Contains(zoneId), -1);

            if (nextZone < 0)
            {
                UpdateAddButtonState(_addButton);
                return;
            }

            AddRow(new ZonePickupPercentage { Zone = nextZone });
            UpdateAddButtonState(_addButton);
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
                    _addButton,
                    actions
                }
            }
        };
    }

    private static View CreateColumnHeadings()
    {
        var grid = CreateGrid();
        string[] labels = { "Zone", "Select %", "Health %", "Ammo %", "Weapon %" };
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

        AttachPercentageValidation(allocation, "Select");
        AttachPercentageValidation(health, "Health");
        AttachPercentageValidation(ammo, "Ammo");
        AttachPercentageValidation(weapon, "Weapon");

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
            HorizontalOptions = LayoutOptions.End,
            IsVisible = item.Zone != 0
        };

        var row = new VerticalStackLayout { Spacing = 4, Children = { grid, remove } };
        var thresholdRow = new ThresholdRow
        {
            Zone = zone,
            AcceptedZoneId = item.Zone,
            Allocation = allocation,
            Health = health,
            Ammo = ammo,
            Weapon = weapon,
            View = row
        };

        zone.Completed += async (_, _) =>
        {
            if (!TryRead(zone.Text, out var enteredZone) ||
                enteredZone < 0 ||
                enteredZone >= _plan.Zones.Count)
            {
                await DisplayAlert(
                    "Unknown zone",
                    $"Zone ID must be between 0 and {_plan.Zones.Count - 1}, matching a zone in the current game plan.",
                    "OK");
                zone.Text = thresholdRow.AcceptedZoneId.ToString();
                return;
            }

            if (_rows.Any(r => !ReferenceEquals(r, thresholdRow) && r.AcceptedZoneId == enteredZone))
            {
                await DisplayAlert(
                    "Duplicate zone",
                    $"A threshold for zone {enteredZone} already exists. The new value has been ignored.",
                    "OK");
                zone.Text = thresholdRow.AcceptedZoneId.ToString();
                return;
            }

            thresholdRow.AcceptedZoneId = enteredZone;
            remove.IsVisible = enteredZone != 0;
            UpdateAddButtonState(_addButton);
        };

        remove.Clicked += (_, _) =>
        {
            if (_rows.Count <= 1 || thresholdRow.AcceptedZoneId == 0)
                return;

            _rows.Remove(thresholdRow);
            _rowsLayout.Children.Remove(row);
            UpdateAddButtonState(_addButton);
        };

        _rows.Add(thresholdRow);
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
                zone < 0 ||
                zone >= _plan.Zones.Count)
            {
                await DisplayAlert(
                    "Invalid values",
                    $"Enter whole numbers, use zone IDs from 0 to {_plan.Zones.Count - 1}, and enter percentages from 0 to 100.",
                    "OK");
                return;
            }

            if (allocation is < 0 or > 100 ||
                health is < 0 or > 100 ||
                ammo is < 0 or > 100 ||
                weapon is < 0 or > 100)
            {
                await DisplayAlert(
                    "Invalid percentage",
                    "Select, Health, Ammo and Weapon percentages must each be between 0 and 100.",
                    "OK");
                return;
            }

            updated.Add(new ZonePickupPercentage
            {
                Zone = zone,
                AllocationPercentage = allocation,
                HealthPercentage = health,
                AmmoPercentage = ammo,
                WeaponPercentage = weapon
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

    private void AttachPercentageValidation(Entry entry, string fieldName)
    {
        var acceptedValue = entry.Text;

        entry.Unfocused += async (_, _) =>
        {
            if (!TryRead(entry.Text, out var value) || value is < 0 or > 100)
            {
                await DisplayAlert(
                    "Invalid percentage",
                    $"{fieldName} must be a whole number between 0 and 100.",
                    "OK");
                entry.Text = acceptedValue;
                return;
            }

            acceptedValue = value.ToString();
        };
    }

    private void UpdateAddButtonState(Button? button)
    {
        if (button is null)
            return;

        var configuredZones = _rows
            .Select(row => row.AcceptedZoneId)
            .ToHashSet();

        button.IsEnabled =
            _plan.Zones.Count > 0 &&
            Enumerable.Range(0, _plan.Zones.Count).Any(zoneId => !configuredZones.Contains(zoneId));
    }

    private static bool TryRead(string? text, out int value) =>
        int.TryParse(text, out value);
}
