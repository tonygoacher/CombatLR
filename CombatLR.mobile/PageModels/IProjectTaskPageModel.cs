using CombatLR.mobile.Models;
using CommunityToolkit.Mvvm.Input;

namespace CombatLR.mobile.PageModels
{
    public interface IProjectTaskPageModel
    {
        IAsyncRelayCommand<ProjectTask> NavigateToTaskCommand { get; }
        bool IsBusy { get; }
    }
}