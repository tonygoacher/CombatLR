
#include "PacedLED.h"


PacedLED::PacedLED(int port)
: _port(port), _pacer(false)
{
    pinMode(_port,OUTPUT);
    digitalWrite(_port,LOW);
}



void PacedLED::Fire(int period)
{
    _pacer.SetPace(period);
    _pacer.PacerReset();
    digitalWrite(_port,HIGH);
    Serial.printf("LED %d ON\n", _port);
}




void PacedLED::Run()
{
    if(_pacer.Pace())
    {
        digitalWrite(_port,LOW);
        Serial.printf("LED %d OFF\n", _port);
    }
}
