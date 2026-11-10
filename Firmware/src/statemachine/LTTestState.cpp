
#include <vector>

#include "statemachine/LTTestState.h"
#include "ISystemTime.h"
#include "graphics/DisplayManager.h"


#include "statemachine/SystemStates.h"
#include "SystemConfig.h"

#include "PBSwitch.h"
#include "ISounds.h"


#include "Player.h"
#include "widgets/PlayerHealthWidget.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "widgets/UIWidgetIds.h"
#include "ESP32LaserTag.h"
#include "PacedLED.h"





LTTestState::LTTestState(CombatLRServices& services)
: _services(services),
 _player(services.sounds, TestModeStartHealth)
{
}


uint16_t LTTestState::Run()
{
    LaserTagData ltData;

    if(_services.laserTag.GetLasertagData(ltData))
    {

        if( ltData.playerId != _services.systemConfig.GetPlayerId())   // Reject player ID that is the same. e.g. reflected own shot
        {
            if(ltData.team != _services.systemConfig.GetTeamId() || _services.systemConfig.GetFriendlyFire())// Team must differ unless friendly fire enabled
            {

                int newHealth = _player.TakeHealth(ShotPower);
                if(newHealth)
                {
                    _services.hitLED.Fire(ShotHitLedTime);
                    _services.sounds.Play("/sounds/hit.wav");  
                }
                else
                {
                    return SystemStates::Dead;
                }

            }                
        }            
    }

    if(_services.triggerSwitch.Pressed())
    {
        LaserTagData tagData;
        tagData.team = _services.systemConfig.GetTeamId();
        tagData.playerId = _services.systemConfig.GetPlayerId();
        tagData.shotPowerIndex = 0; // Unused, it's hard coded in this class
        _services.laserTag.SetIRPower(IRPowerLevel::IRPowerHigh);
        _services.muzzleLED.Fire(300);
        _services.sounds.PlayCached();
        _services.laserTag.SendLasertagData(tagData);
    }


    return SystemStates::IRTestState;  

}



void LTTestState::Entry()
{


    _player.InitHealth();
    _services.sounds.CacheSample("/sounds/blaster.wav");

    PlayerHealthWidget*  healthDisplay = new PlayerHealthWidget(UIWidgetIds::PlayerHealth, _player, HealthX, HealthY, HealthW, HealthH);

    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(healthDisplay)));

    while(_services.triggerSwitch.RawRead())
    {
        delay(100);
    }                
}




void LTTestState::Exit()
{

    _services.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(UIWidgetIds::PlayerHealth)));

}

