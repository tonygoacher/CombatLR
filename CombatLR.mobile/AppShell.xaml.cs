using CommunityToolkit.Maui.Alerts;

using CombatLR.mobile.Pages;
using CombatLR.mobile.Services;

namespace CombatLR.mobile;

public partial class AppShell : Shell
{
    public AppShell(SelectedGamePlanService selectedGamePlan)
    {
        InitializeComponent();
        RootContent.Content = new GamePlanSelectionPage(selectedGamePlan);
    }

    public static async Task DisplayToastAsync(string message)
    {
        await Toast.Make(message).Show();
    }
}