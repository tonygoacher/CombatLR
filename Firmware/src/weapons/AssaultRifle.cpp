#include "weapons/AssaultRifle.h"
#include "IRLaserTag.h"
#include "PBSwitch.h"
#include "Texture.h"
#include "ISounds.h"



namespace ARSamples
{
    static const char * const Shot = "/sounds/milshot.wav";
    static const char * const Empty = "/sounds/empty.wav";
    static const char * const InsertClip = "/sounds/insclip.wav";
    static const char * const Reload = "/sounds/reload.wav";

};


AssaultRifle::AssaultRifle( uint16_t reserve, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds)
: Weapon(reserve, magSize, laserTag, trigger, reload, sounds),
_rpmPacer(true),
_reloadPacer(true),
_state(ARState::Idle)
{
    _rpmPacer.SetPace(150);

}


void AssaultRifle::Run()
{
    switch(_state)
    {
        case ARState::Idle:
        {
            if(_trigger.RawRead())
            {                 

                if(_magAmmo)
                {
                    _sounds.PlayCached(ARSamples::Shot);
                    _magAmmo--;
                   
                    _state = ARState::Shooting;
                    _rpmPacer.PacerReset();
                }
                else
                {
                    _sounds.Play(ARSamples::Empty);
                    _rpmPacer.PacerReset();    
                    _state = ARState::Empty;             // USe the rpm pacer to debounce the raw trigger       
                }

            }
            else
            {
                if(_reload.Pressed())
                {
                    _state = ARState::MagInsert;
                    _reloadPacer.SetPace(3000);
                    _sounds.Play(ARSamples::InsertClip);
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
            if(!_trigger.RawRead() && _rpmPacer.Pace())
            {
                _state = ARState::Idle;
            }
        }
        break;


        case ARState::MagInsert:
        {
            if(_reloadPacer.Pace())
            {
                _sounds.Play(ARSamples::Reload);
                _state = ARState::Cocking;
            }
        }
        break;

        case ARState::Cocking:
        {
            if(_sounds.HasFinished())
            {
                _state = ARState::Idle;
                CompleteReload();
            }
        }
        break;

        default: 
        {
            Serial.println("AR invalid state");
        }   
    }

    
}



void AssaultRifle::AddAmmo(uint16_t addAmmo)
{

}


void AssaultRifle::Start()
{
    _sounds.CacheSample(ARSamples::Shot);
    _texture 
}

void AssaultRifle::Stop()
{
    _sounds.UnCacheSample(ARSamples::Shot);
}