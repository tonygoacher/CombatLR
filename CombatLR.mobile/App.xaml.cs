using Microsoft.Extensions.DependencyInjection;

using CombatLR.mobile.Services;

namespace CombatLR.mobile
{
    public partial class App : Application
    {
        private readonly SelectedGamePlanService _selectedGamePlan;

        public App(SelectedGamePlanService selectedGamePlan)
        {
            InitializeComponent();
            _selectedGamePlan = selectedGamePlan;
        }

        protected override Window CreateWindow(IActivationState? activationState)
        {
            return new Window(new AppShell(_selectedGamePlan));
        }
    }
}