#include <Arduino.h>

#define	SWITCH0	2



#include <Arduino.h>

#include "PBSwitch.h"


PBSwitch::PBSwitch()
{



}

PBSwitch::PBSwitch(uint8_t port)
: m_Port(port)
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
				Serial.println("AAAA");
				SetTimer();
			}
		}
		break;

		case PBSwitch::PRESSED_HOLD:
		{
			if (!ReadPort() && Elapsed() > sm_DebounceTime)
			{
				m_SwitchState = PBSwitch::OFF_HOLD;
				SetTimer();
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