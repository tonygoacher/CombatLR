using Android.App;
using Android.Content.PM;

using Android.OS;
using AndroidX.Browser.Trusted;

namespace CombatLR.mobile
{
    [Activity(
        Theme = "@style/Maui.SplashTheme", 
        MainLauncher = true, 
        LaunchMode = LaunchMode.SingleTop,
        ScreenOrientation = Android.Content.PM.ScreenOrientation.Landscape,
        ConfigurationChanges = ConfigChanges.ScreenSize | ConfigChanges.Orientation | ConfigChanges.UiMode | ConfigChanges.ScreenLayout | ConfigChanges.SmallestScreenSize | ConfigChanges.Density),
        ]
    public class MainActivity : MauiAppCompatActivity
    {
    }
}
