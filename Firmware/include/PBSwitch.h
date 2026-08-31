#pragma once
#include "stdint.h"
class PBSwitch
{
public:
	PBSwitch(bool oneShot);
	PBSwitch(uint8_t port, bool oneShot);
	void Init(uint8_t port);
	bool Pressed();
private:
	unsigned long m_Timer;
	uint8_t m_Port;
	bool m_oneShot;
	static const unsigned long sm_DebounceTime = 40;
	enum SwitchState
	{
		OFF,
		PRESSED,
		PRESSED_HOLD,
		OFF_HOLD,
	};

	SwitchState	m_SwitchState;

	unsigned long Elapsed();
	bool ReadPort();
	void SetTimer();

};