

#include "statemachine/LTTestState.h"
#include "statemachine/SystemStates.h"
#include "ISystemTime.h"
#include "graphics/DisplayManager.h"
#include "TeamIDs.h"
#include "graphics/TextureManager.h"
#include "widgets/TextureWidget.h"


#include "weapons/WeaponManager.h"
#include "SystemConfig.h"

#include "PBSwitch.h"
#include "ISounds.h"


#include "Player.h"
#include "widgets/PlayerHealthWidget.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "ESP32LaserTag.h"
#include "PacedLED.h"
#include "widgets/UIWidgetIds.h"





LTTestState::LTTestState(CombatLRServices& services, WeaponManager& weaponManager)
: _services(services),
 _player(services.sounds),
 _weaponManager(weaponManager)
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

    _weaponManager.Run();


    return SystemStates::IRTestState;  

}



void LTTestState::Entry()
{
    _services.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/ltmode.bmp")));

    _weaponManager.Init();

    _weaponManager.AddWeapon(DefaultWeaponIndex);
    _weaponManager.SetAmmo(DefaultWeaponIndex,999);
  
    _player.InitHealth(DefaultStartHealth,DefaultStartHealth);

    PlayerHealthWidget*  healthDisplay = new PlayerHealthWidget(UIWidgetIds::PlayerHealth, _player, HealthX, HealthY, HealthW, HealthH);

    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(healthDisplay)));

    WidgetPosition posTeam(160,0);

    TeamIds team = static_cast<TeamIds>(_services.systemConfig.GetTeamId());
    TextureWidget* teamIcon  = new TextureWidget(UIWidgetIds::TeamID,
        _services.displayManager.GetTextureManager(),posTeam,_services.systemConfig.GetTeamFile(team));

    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(teamIcon));                     

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

