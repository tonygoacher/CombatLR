using CommunityToolkit.Maui.Alerts;

namespace CombatLR.mobile;

public partial class AppShell : Shell
{
    public AppShell()
    {
        InitializeComponent();
    }

    public static async Task DisplayToastAsync(string message)
    {
        await Toast.Make(message).Show();
    }
}