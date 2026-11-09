
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/LTTestState.h"
#include "ESP32LaserTag.h"
#include "PacedLED.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "graphics/DisplayManager.h"
#include "widgets/TextWidget.h"
#include "widgets/UIWidgetIds.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/SetTextCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "statemachine/GenericEvent.h"
#include "PBSwitch.h"
#include "SystemConfig.h"
#include "IRPowerLevel.h"
#include "statemachine/SystemStates.h"
#include "ISounds.h"




LTTestState::LTTestState(CombatLRServices& services)
: _services(services)
{

}

void LTTestState::Entry()
{
    _services.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/logo.bmp")));

    WidgetPosition textPos={TextX,TextY};

    TextWidget*  textWidget  = new TextWidget(UIWidgetIds::TestModeIndicator, "Test Mode", textPos,TextFont,MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom);
    
    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(textWidget)));            
 
}




void LTTestState::Exit()
{
   
    RemoveWidgetCommand* removeIcon = new RemoveWidgetCommand(UIWidgetIds::TestModeIndicator);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));   
}




uint16_t LTTestState::Run()
{
     LaserTagData ltData;

    if(_services.laserTag.GetLasertagData(ltData))
    {
        if(ltData.playerId != _services.systemConfig.GetPlayerId())
        {
            _services.sounds.Play("/sounds/hit.wav");
        }
    }   
    
    if(_services.triggerSwitch.Pressed())   // This is single shot, switch must be released before this returns true again
    {
        LaserTagData tagData;

        tagData.team = _services.systemConfig.GetTeamId();
        tagData.playerId = _services.systemConfig.GetPlayerId();
        tagData.shotPowerIndex = DefaultWeaponIndex;
        _services.laserTag.SetIRPower(IRPowerLevel::IRPowerHigh);

        _services.laserTag.SendLasertagData(tagData);
        _services.muzzleLED.Fire(InterShotTime);
    }

   
    return SystemStates::IRTestState;
}




