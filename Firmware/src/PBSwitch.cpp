#include <Arduino.h>

#define	SWITCH0	2



#include <Arduino.h>

#include "PBSwitch.h"


PBSwitch::PBSwitch(bool oneShot)
: m_oneShot(oneShot)
{


}

PBSwitch::PBSwitch(uint8_t port, bool oneShot)
: m_Port(port), m_oneShot(oneShot)
{

	pinMode(port, INPUT_PULLUP);
	m_SwitchState = PBSwitch::OFF;
}

unsigned long PBSwitch::Elapsed()
{
	return millis() - m_Timer;
}

void PBSwitch::Init(uint8_t port)
{
	m_Port = port;
	pinMode(port, INPUT_PULLUP);
	m_SwitchState = PBSwitch::OFF;
}

bool PBSwitch::ReadPort()
{
	int value = digitalRead(m_Port);

	return value == LOW;
}

void PBSwitch::SetTimer()
{
	m_Timer = millis();
}

bool PBSwitch::Pressed()
{
	bool pressed = false;
	switch (m_SwitchState)
	{
		case PBSwitch::OFF:
		{
			if (ReadPort())
			{
				pressed = true;
				m_SwitchState = PBSwitch::PRESSED_HOLD;
				SetTimer();
			}
		}
		break;

		case PBSwitch::PRESSED_HOLD:
		{
			if(Elapsed() < sm_DebounceTime)
			{
				pressed = !m_oneShot;		// If not in one shot mode, keep the switch returning pressed during the debounce.
			}
			else
			{
				if(ReadPort())
				{
					pressed = (!m_oneShot);			// Now out of debounce, if switch is still down, report it as such if not in oneShot
				}
				else
				{
					m_SwitchState = PBSwitch::OFF_HOLD;	// Switch is no longer down, go to up debounce
					SetTimer();
				}
			}
			
		}
		break;

		case PBSwitch::OFF_HOLD:
		{
			if (!ReadPort() && Elapsed() > sm_DebounceTime)	
			{
				m_SwitchState = PBSwitch::OFF;
			}
		}
		break;
	}

	return pressed;
}