#pragma once
#include "stdint.h"
class PBSwitch
{
public:
	PBSwitch(bool oneShot);
	PBSwitch(uint8_t port, bool oneShot);
	void Init(uint8_t port);
	bool Pressed();
	bool RawRead();
private:
	unsigned long m_Timer;
	uint8_t m_Port;
	bool m_oneShot;
	static const unsigned long sm_DebounceTime = 30;
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