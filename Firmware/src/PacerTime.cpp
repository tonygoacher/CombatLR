

#include "PacerTime.h"


PacerTime::PacerTime()
:
    _pacer(true),
    _gameTime(0)
{
    _pacer.SetPace(1000);   
}

void PacerTime::Reset()
{
    _gameTime = 0;
    _pacer.PacerReset();  
}

uint32_t PacerTime::GetGameTime() 
{
        if(_pacer.Pace())
        {
            _gameTime++;
        }
        return _gameTime;
}
