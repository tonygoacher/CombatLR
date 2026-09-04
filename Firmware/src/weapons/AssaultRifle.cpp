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
    static const char * const ARImage = "/weapons/assault.bmp";

};


AssaultRifle::AssaultRifle( uint16_t reserve, uint16_t magSize, CombatLRServices& services)
: Weapon(reserve, magSize, services),
_rpmPacer(true),
_reloadPacer(true),
_state(ARState::Idle)
{
    _rpmPacer.SetPace(150);

}


bool AssaultRifle::Run()
{
    bool actionTaken = false;
    switch(_state)
    {
        case ARState::Idle:
        {
            if(_services.triggerSwitch.RawRead())
            {                 

                if(_magAmmo)
                {
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
            else
            {
                if(_services.reloadSwitch.Pressed())
                {
                    _state = ARState::MagInsert;
                    _reloadPacer.SetPace(3000);
                    _services.sounds.Play(ARStrings::InsertClip);
                }
            }
        }
        break;

        case ARState::Shooting:
        {
            if(_rpmPacer.Pace())
            {
                _state = ARState::Idle;
            }
        }
        break;

        case ARState::Empty:
        {
            if(!_services.triggerSwitch.RawRead() && _rpmPacer.Pace())
            {
                _state = ARState::Idle;
            }
        }
        break;


        case ARState::MagInsert:
        {
            if(_reloadPacer.Pace())
            {
                _services.sounds.Play(ARStrings::Reload);
                _state = ARState::Cocking;
            }
        }
        break;

        case ARState::Cocking:
        {
            if(_services.sounds.HasFinished())
            {
                _state = ARState::Idle;
                actionTaken = true;
                CompleteReload();
            }
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
    return ARStrings::ARImage;
}
