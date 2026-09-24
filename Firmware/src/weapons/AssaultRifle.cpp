#include "weapons/AssaultRifle.h"
#include "IRLaserTag.h"
#include "PBSwitch.h"
#include "graphics/Texture.h"
#include "ISounds.h"
#include "stateMachine/Services.h"
#include "graphics/DisplayManager.h"
#include "weapons/WeaponConfig.h"
#include "countof.h"
#include "PacedLED.h"



AssaultRifle::AssaultRifle( WeaponType type, WeaponConfig& weaponConfig,CombatLRServices& services)
: Weapon( type, weaponConfig,services),
_rpmPacer(true),
_reloadPacer(true),
_state(ARState::StartReload),

_shotsTaken(0)
{
    _burst = _weaponConfig.GetBurst(type);

}



bool AssaultRifle::IdleAction()
{
    bool actionTaken = false;
    if(_services.triggerSwitch.Pressed())
    {   Serial.printf("Shots taken %d Burst %d\n", _shotsTaken, _burst);             
        if(_burst==-1 ||    ( _shotsTaken < _burst))
        {
            if(_magAmmo)
            {
                _services.sounds.PlayCached();
                if(_weaponConfig.GetBuildUpTime(_type) == 0) // Weapon has no build up
                {
                
                    _shotsTaken++;
                    _magAmmo--;

                
                    _state = ARState::Shoot;
                
                    actionTaken = true;
                } 
                else
                {
                        _rpmPacer.SetPace(_weaponConfig.GetBuildUpTime(_type));
                        _state = ARState::BuildUp;
                }

            }
            else
            {
                _services.sounds.Play(GetEmptyFilename());
                _rpmPacer.PacerReset();    
                _state = ARState::Empty;             // USe the rpm pacer to debounce the raw trigger       
            }
        }                    

    }
    else
    {
        _shotsTaken = 0;
        if(_services.reloadSwitch.Pressed())
        {
            _state=ARState::StartReload;
            
        }
    }
    return actionTaken;
}


void AssaultRifle::StartReload()
{
    _shotsTaken = 0;

    _state = ARState::MagInsert;
    _reloadPacer.SetPace(_weaponConfig.GetInsertTime(_type));
    _services.sounds.Play(GetMagInsertFilename());
}

void AssaultRifle::Shoot()
{
    _rpmPacer.PacerReset();
    int muzzleLEDTime = _weaponConfig.GetMuzzleLEDTime(_type);
    if(muzzleLEDTime)
    {
        _services.muzzleLED.Fire(muzzleLEDTime);
    }     
    _state = ARState::ShootWait;  
}

void AssaultRifle::ShootWait()
{
    if(_rpmPacer.Pace())
    {
        _state = ARState::Idle;
    }
}


bool AssaultRifle::BuildUpAction()
{
    bool actionTaken = false;
    if(_rpmPacer.Pace())
    {
        _shotsTaken++;
        _magAmmo--;

    
        _state = ARState::Shoot;
        _rpmPacer.SetPace(_weaponConfig.GetPostBuildUpShotTime(_type));
        actionTaken = true;
    }

    if(!_services.triggerSwitch.Pressed())  // If the trigger has been released, stop the build up
    {
        _services.sounds.StopCached();
        _state = ARState::Idle;
    }

    return actionTaken;
}



void AssaultRifle::EmptyAction()
{
    if(!_services.triggerSwitch.RawRead() && _rpmPacer.Pace())
    {
        _state = ARState::Idle;
    }
}



void AssaultRifle::MagInsertAction()
{
    if(_reloadPacer.Pace())
    {
        _services.sounds.Play(GetReloadFilename());
        _state = ARState::Cocking;
        _reloadPacer.SetPace(_weaponConfig.GetCockingTime(_type));
    }
}


bool AssaultRifle::CockingAction()
{
    bool actionTaken = false;
    if(_reloadPacer.Pace())
    {
        _state = ARState::Idle;
        actionTaken = true;
        CompleteReload();
    }
    return actionTaken;
}



void AssaultRifle::EquippingAction()
{
    if(_services.sounds.HasFinished() && !_services.triggerSwitch.RawRead() && !_services.reloadSwitch.RawRead())
    {
        _state = ARState::Idle;
    }
}



bool AssaultRifle::Run()
{
    bool actionTaken = false;
    switch(_state)
    {
        case ARState::Idle:
        {
            actionTaken = IdleAction();
        }
        break;

        case ARState::ShootWait:
        {
            ShootWait();
        }
        break;

        case ARState::Shoot:
        {
            Shoot();
        }
        break;

        case ARState::BuildUp:
        {
            actionTaken = BuildUpAction();
        }
        break;

        case ARState::Empty:
        {
            EmptyAction();
        }
        break;

         case ARState::StartReload:
        {
            StartReload();
        }
        break;


        case ARState::MagInsert:
        {
            MagInsertAction();
        }
        break;

        case ARState::Cocking:
        {
            actionTaken = CockingAction();
        }
        break;

        case ARState::Equipping:
        {
            EquippingAction();
        }
        break;


        default: 
        {
            Serial.println("AR invalid state");
        }   
    }

    return actionTaken;  
}



void AssaultRifle::AddAmmo(uint16_t addAmmo)
{


    _services.sounds.Play("/sounds/ampick.wav");   
    _reserveAmmo += addAmmo;

}


void AssaultRifle::Start()
{
    Serial.println(GetShotFilename());
    if(GetShotFilename() != nullptr)
    {
        _services.sounds.CacheSample(GetShotFilename());
    }        

}

void AssaultRifle::Stop()
{
    _services.sounds.UnCacheSample();
}



const char * const AssaultRifle::GetFilename() 
{
    return _weaponConfig.GetImageFilename(_type);  
}



void AssaultRifle::Equip()
{

    _services.sounds.Play(GetReloadFilename());
    _state =  ARState::Equipping;
}



const char * const AssaultRifle::GetShotFilename()
{
    return _weaponConfig.GetShootSampleFilename(_type);
}



const char * const AssaultRifle::GetReloadFilename()
{
    return _weaponConfig.GetCockingFilename(_type);
}



const char * const AssaultRifle::GetEmptyFilename()
{
    return _weaponConfig.GetEmptyFilename(_type);
}

const char * const AssaultRifle::GetMagInsertFilename()
{
    return _weaponConfig.GetClipInsertSampleFilename(_type);
}
