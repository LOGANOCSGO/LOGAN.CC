
#pragma once
#include "RageBot.h"
#include "Hacks.h"
extern Vector AimPoint;

extern int hittedLogHits[65];
extern int missedLogHits[65];
extern int ResolverMode;
extern float enemyLBYTimer[65];
extern float enemysLastProxyTimer[65];
extern float enemyLBYDelta[65];
extern float lineLBY;
extern float lineRealAngle;
extern float lineFakeAngle;
extern int ResolverStage[65];
extern float autowalldmgtest[65];

class CEsp : public CHack
{
public:
	void Init();
	void Draw();
	void Move(CUserCmd *pCmd, bool &bSendPacket);
private:

	// Other shit

	struct ESPBox
	{
		float x, y, w, h;
	};

	// Draw a player
	void DrawPlayer(IClientEntity* pEntity, player_info_t pinfo);

	// Get player info
	Color GetPlayerColor(IClientEntity* pEntity);
	bool GetBox(IClientEntity* pEntity, ESPBox &result);

	// Draw shit about player
	void SpecList();
	void DrawBox(ESPBox size, Color color);
	void DrawName(player_info_t pinfo, ESPBox size);
	void DrawHealth(IClientEntity* pEntity, CEsp::ESPBox size);
	void DrawInfo(IClientEntity* pEntity, ESPBox size);
	void DrawCross(IClientEntity* pEntity);
	void drawBacktrackedSkelet(IClientEntity *base , player_info_t);
	void DrawSkeleton(IClientEntity* pEntity);
	void BoxAndText(IClientEntity * entity, std::string text);
	void DrawDrop(IClientEntity* pEntity, ClientClass* cClass);
	void DrawBombPlanted(IClientEntity* pEntity, ClientClass* cClass);
	void DrawBombPlanted2(IClientEntity * pEntity, ClientClass * cClass);
	void DefuseWarning(IClientEntity * pEntity);
	void DrawBomb(IClientEntity* pEntity, ClientClass* cClass);
	void SightLines(IClientEntity * pEntity, Color color);
};