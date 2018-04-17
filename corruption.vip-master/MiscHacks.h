
#pragma once

#include "Hacks.h"

Vector GetAutostrafeView();

class CMiscHacks : public CHack
{
public:
	void Init();
	void Draw();
	void Move(CUserCmd *pCmd, bool &bSendPacket);
	void RotateMovement(CUserCmd * pCmd, float rotation);
private:
	void AutoJump(CUserCmd *pCmd);
	void FakeWalk(CUserCmd* pCmd, bool &bSendPacket);
	void LegitStrafe(CUserCmd *pCmd);
	void RageStrafe(CUserCmd *pCmd);
	void ChatSpamInterwebz();
	void ChatSpamName();
	void ChatSpamDisperseName();
	void ChatSpamRegular();
};



