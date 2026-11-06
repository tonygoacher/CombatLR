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

            GameProgressButton.IsEnabled =
                _selectedGamePlan.GamePlan is not null &&
                new GameSessionService().GetSession(
                    _selectedGamePlan.ProjectName,
                    _selectedGamePlan.GamePlan.Version) is not null;
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
                new GameControlPage(_selectedGamePlan));
        }

        private async void GameProgress_Clicked(
            object? sender,
            EventArgs e)
        {
            var plan = _selectedGamePlan.GamePlan;
            var session = plan is null
                ? null
                : new GameSessionService().GetSession(
                    _selectedGamePlan.ProjectName,
                    plan.Version);

            if (session is null)
            {
                await DisplayAlert(
                    "Game Not Started",
                    "Game Progress is available after a CombatLR device acknowledges START GAME for this plan.",
                    "OK");
                UpdateSelectedGamePlanLabel();
                return;
            }

            await Navigation.PushAsync(
                new GameProgressPage(_selectedGamePlan));
        }

        private async void ProjectUpdate_Clicked(
            object? sender,
            EventArgs e)
        {
            if (!_selectedGamePlan.HasSavedGamePlan)
            {
                await DisplayAlert(
                    "No Game Plan",
                    "This project does not have a saved game plan. Open the Game Plan Editor and save a plan before updating devices.",
                    "OK");
                return;
            }

            await Navigation.PushAsync(
                new ProjectUpdatePage(_selectedGamePlan));
        }
    }
}
