using Microsoft.Maui.Controls;

using CombatLR.mobile.Services;

namespace CombatLR.mobile.Pages
{
    public partial class MainPage : ContentPage
    {
        private readonly SelectedGamePlanService _selectedGamePlan;

        public MainPage(SelectedGamePlanService selectedGamePlan)
        {
            InitializeComponent();
            _selectedGamePlan = selectedGamePlan;
            UpdateSelectedGamePlanLabel();
        }

        protected override void OnAppearing()
        {
            base.OnAppearing();
            UpdateSelectedGamePlanLabel();
        }

        private void UpdateSelectedGamePlanLabel()
        {
            SelectedGamePlanLabel.Text = _selectedGamePlan.GamePlan is null
                ? "No game plan selected"
                : $"PLAN: {_selectedGamePlan.ProjectName}  v{_selectedGamePlan.GamePlan.Version}";
        }

        private async void GamePlanEditor_Clicked(
            object? sender,
            EventArgs e)
        {
            await Navigation.PushAsync(
                new GamePlanEditorPage(_selectedGamePlan));
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
                new ProjectUpdatePage(_selectedGamePlan));
        }
    }
}
