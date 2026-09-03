#include "weapons/AssaultRifle.h"
#include "IRLaserTag.h"
#include "PBSwitch.h"
#include "Texture.h"
#include "ISounds.h"



namespace ARSamples
{
    static const char * const Shot = "/sounds/milshot.wav";

};


AssaultRifle::AssaultRifle( uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds)
: Weapon(ammo, magSize, laserTag, trigger, reload, sounds),
_rpmPacer(true),
_reloadPacer(false),
_state(ARState::Idle)
{
    _rpmPacer.SetPace(150);

}


struct timespec start_time, end_time;
double interval_ms;
void AssaultRifle::Run()
{
    switch(_state)
    {
        case ARState::Idle:
        {
            if(_trigger.RawRead())
            {                 

                clock_gettime(CLOCK_MONOTONIC, &end_time);
                 interval_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                      (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

                Serial.printf("Interval %0.2f\n", interval_ms);   
                if(_magAmmo)
                {
                    _sounds.PlayCached(ARSamples::Shot);
                    _magAmmo--;
                   
                    _state = ARState::Shooting;
                    _rpmPacer.PacerReset();
                }
                else
                {
                    Serial.println("Assault rifle empty");
                }
                clock_gettime(CLOCK_MONOTONIC, &start_time);
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
}

void AssaultRifle::Stop()
{
    _sounds.UnCacheSample(ARSamples::Shot);
}