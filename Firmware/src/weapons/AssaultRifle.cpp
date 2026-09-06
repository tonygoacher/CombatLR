#include "weapons/AssaultRifle.h"
#include "IRLaserTag.h"
#include "PBSwitch.h"
#include "Texture.h"
#include "ISounds.h"
#include "stateMachine/Services.h"
#include "DisplayManager.h"



namespace ARStrings
{
    static const char * const Shot = "/sounds/milshot.wav";
    static const char * const Empty = "/sounds/empty.wav";
    static const char * const InsertClip = "/sounds/insclip.wav";
    static const char * const Reload = "/sounds/reload.wav";
    static const char * const ARImageFullAuto = "/weapons/assault.bmp";
    static const char * const ARImage = "/weapons/sa80.bmp";

};


AssaultRifle::AssaultRifle( WeaponType type, CombatLRServices& services, int burst)
: Weapon( type, services),
_rpmPacer(true),
_reloadPacer(true),
_state(ARState::StartReload),
_burst(burst),
_shotsTaken(0)
{

    _rpmPacer.SetPace(150);

}



bool AssaultRifle::IdleAction()
{
    bool actionTaken = false;
    Serial.println(static_cast<int>(_state));
    if(_services.triggerSwitch.RawRead())
    {                 
        if(_burst==-1 ||    ( _shotsTaken < _burst))
        {
            if(_magAmmo)
            {
                _shotsTaken++;
                _services.sounds.PlayCached(ARStrings::Shot);
                _magAmmo--;
            
                _state = ARState::Shooting;
                _rpmPacer.PacerReset();
                actionTaken = true;
            }
            else
            {
                _services.sounds.Play(ARStrings::Empty);
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
    _reloadPacer.SetPace(3000);
    _services.sounds.Play(ARStrings::InsertClip);
}


void AssaultRifle::ShootingAction()
{
    if(_rpmPacer.Pace())
    {
        _state = ARState::Idle;
    }
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
        _services.sounds.Play(ARStrings::Reload);
        _state = ARState::Cocking;
    }
}


bool AssaultRifle::CockingAction()
{
    bool actionTaken = false;
    if(_services.sounds.HasFinished())
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

        case ARState::Shooting:
        {
            ShootingAction();
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
    _services.sounds.CacheSample(ARStrings::Shot);

}

void AssaultRifle::Stop()
{
    _services.sounds.UnCacheSample(ARStrings::Shot);
}



const char * const AssaultRifle::GetFilename() 
{
    if(_burst == -1)
    {
        return ARStrings::ARImageFullAuto;
    }
    else
    {
        return ARStrings::ARImage;

    }
}



void AssaultRifle::Equip()
{
    _services.sounds.Play(ARStrings::Reload);
    _state =  ARState::Equipping;
}
