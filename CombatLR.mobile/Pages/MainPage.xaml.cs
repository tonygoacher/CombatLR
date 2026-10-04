using Microsoft.Maui.Controls;

namespace CombatLR.mobile.Pages
{
    public partial class MainPage : ContentPage
    {
        public MainPage()
        {
            InitializeComponent();
        }

        private async void GamePlanEditor_Clicked(
            object? sender,
            EventArgs e)
        {
            await Navigation.PushAsync(
                new GamePlanEditorPage());
        }

        private async void GameControl_Clicked(
            object? sender,
            EventArgs e)
        {
            await Navigation.PushAsync(
                new GameControlPage());
        }

        private async void ProjectUpdate_Clicked(
            object? sender,
            EventArgs e)
        {
            await Navigation.PushAsync(
                new ProjectUpdatePage());
        }
    }
}
