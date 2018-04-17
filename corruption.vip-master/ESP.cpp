
#include "ESP.h"
#include "Interfaces.h"
#include "RenderManager.h"
#include "Glowmanager.h"
#include "RageBot.h"
#include "SDK.h"
#include "lagcomp.h"
#include "Autowall.h"

DWORD GlowManager = *(DWORD*)(Utilities::Memory::FindPatternV2("client.dll", "0F 11 05 ?? ?? ?? ?? 83 C8 01 C7 05 ?? ?? ?? ?? 00 00 00 00") + 3);
IClientEntity *BombCarrier;
void CEsp::Init()
{
	BombCarrier = nullptr;
}

// Yeah dude we're defo gunna do some sick moves for the esp yeah
void CEsp::Move(CUserCmd *pCmd, bool &bSendPacket)
{

}

// Main ESP Drawing loop
void CEsp::Draw()
{
	IClientEntity *pLocal = hackManager.pLocal();
	if (Menu::Window.VisualsTab.OtherSpectators.GetState())//no reason to put it in 2 diff loops
	{
		SpecList();
	}
	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		player_info_t pinfo;

		if (pEntity &&  pEntity != pLocal && !pEntity->IsDormant())
		{
			if (Menu::Window.VisualsTab.OtherRadar.GetState())
			{
				ptrdiff_t m_bSpotted = 0x939;
				*(char*)((DWORD)(pEntity)+m_bSpotted) = 1;
			}

			if (Menu::Window.VisualsTab.Active.GetState() && Interfaces::Engine->GetPlayerInfo(i, &pinfo) && pEntity->IsAlive())
			{
				DrawPlayer(pEntity, pinfo);
			}

			ClientClass* cClass = (ClientClass*)pEntity->GetClientClass();

			if (Menu::Window.VisualsTab.FiltersWeapons.GetState() && cClass->m_ClassID != (int)CSGOClassID::CBaseWeaponWorldModel && ((strstr(cClass->m_pNetworkName, "Weapon") || cClass->m_ClassID == (int)CSGOClassID::CDeagle || cClass->m_ClassID == (int)CSGOClassID::CAK47)))
			{
				DrawDrop(pEntity, cClass);
			}

			if (Menu::Window.VisualsTab.FiltersC4.GetState())
			{
				if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
					DrawBombPlanted2(pEntity, cClass);

				if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
					DrawBombPlanted(pEntity, cClass);

				if (cClass->m_ClassID == (int)CSGOClassID::CC4)
					DrawBomb(pEntity, cClass);

				if (Menu::Window.VisualsTab.OptionsPlant.GetState())
					DefuseWarning(pEntity);
			}

		}
	}
	if (Menu::Window.VisualsTab.OtherNoFlash.GetState())
	{
		DWORD m_flFlashMaxAlpha = NetVar.GetNetVar(0xFE79FB98);
		*(float*)((DWORD)pLocal + m_flFlashMaxAlpha) = 0;
	}
	else
	{
		DWORD m_flFlashMaxAlpha = NetVar.GetNetVar(0xFE79FB98);
		*(float*)((DWORD)pLocal + m_flFlashMaxAlpha) = 255;
	}
}

void CEsp::SpecList()
{

	IClientEntity *pLocal = hackManager.pLocal();

	RECT scrn = Render::GetViewport();
	static int AC = 0;

		player_info_t pinfo;
		Render::Text(scrn.left, (scrn.bottom / 2 - 16), Color(255, 255, 255, 255), Render::Fonts::ESP, "SPECTATORS:");
		if (pLocal->IsAlive())
		{
			for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
			{
				IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
				if (pEntity == nullptr)
					return;
				if (Interfaces::Engine->GetPlayerInfo(i, &pinfo) && !pEntity->IsAlive() && !pEntity->IsDormant())
				{

					HANDLE obs = pEntity->GetObserverTargetHandle();

					if (obs)
					{
						IClientEntity *pTarget = Interfaces::EntList->GetClientEntityFromHandle(obs);
						player_info_t pinfo2;
						if (pTarget)
						{
							if (Interfaces::Engine->GetPlayerInfo(pTarget->GetIndex(), &pinfo2))
							{
								if (strlen(pinfo.name) > 16 && strlen(pinfo2.name) > 16)
								{
									pinfo.name[16] = 0;
									strcat(pinfo.name, "...");
									puts(pinfo.name);
									pinfo2.name[16] = 0;
									strcat(pinfo2.name, "...");
									puts(pinfo2.name);
									char buf[256]; sprintf_s(buf, "%s", pinfo.name, pinfo2.name);
									RECT TextSize = Render::GetTextSize(Render::Fonts::ESP, buf);
									//Render::Clear(scrn.left, (scrn.bottom / 3) + (16 * AC), 260, 16, Color(0, 0, 0, 140));
									//Render::Text(scrn.left, (scrn.bottom / 2 - 16), pTarget->GetIndex() == pLocal->GetIndex() ? Color(240, 70, 80, 255) : Color(255, 255, 255, 0), Render::Fonts::ESP, "Spectating me list:");
									Render::Text(scrn.left, (scrn.bottom / 2) + (16 * AC), pTarget->GetIndex() == pLocal->GetIndex() ? Color(240, 25, 25, 255) : Color(255, 255, 255, 0), Render::Fonts::ESP, buf);
									AC++;
								}
								else {
									char buf[256]; sprintf_s(buf, "%s", pinfo.name, pinfo2.name);
									RECT TextSize = Render::GetTextSize(Render::Fonts::ESP, buf);
									//Render::Clear(scrn.left, (scrn.bottom / 3) + (16 * AC), 260, 16, Color(0, 0, 0, 140));
									//Render::Text(scrn.left, (scrn.bottom / 2 - 16), pTarget->GetIndex() == pLocal->GetIndex() ? Color(240, 70, 80, 255) : Color(255, 255, 255, 0), Render::Fonts::ESP, "Spectating me list:");
									Render::Text(scrn.left, (scrn.bottom / 2) + (16 * AC), pTarget->GetIndex() == pLocal->GetIndex() ? Color(240, 25, 25, 255) : Color(255, 255, 255, 0), Render::Fonts::ESP, buf);
									AC++;
								}
							}
						}
					}
				}
			}
		}
	//Render::Outline(scrn.right - 261, (scrn.bottom / 2) - 1, 262, (16 * AC) + 2, Color(23, 23, 23, 255));
	//Render::Outline(scrn.right - 260, (scrn.bottom / 2), 260, (16 * AC), Color(90, 90, 90, 255));
}

//  Yeah m8
void CEsp::DrawPlayer(IClientEntity* pEntity, player_info_t pinfo)
{
	ESPBox Box;
	Color Color;
	IClientEntity *pLocal = hackManager.pLocal();

	// Show own team false? well gtfo teammate lol // should fix a stupid fucking esp box issue
	if (Menu::Window.VisualsTab.FiltersEnemiesOnly.GetState() && (pEntity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()))
		return;

	if (pEntity != pLocal && pEntity)
	{
		if (GetBox(pEntity, Box))
		{
			Color = GetPlayerColor(pEntity);

			if (Menu::Window.VisualsTab.OptionsBox.GetState())
				DrawBox(Box, Color);

			if (Menu::Window.VisualsTab.OptionsName.GetState())
				DrawName(pinfo, Box);

			if (Menu::Window.VisualsTab.OptionsHealth.GetState())
				DrawHealth(pEntity, Box);

			if (Menu::Window.VisualsTab.OptionsInfo.GetState() || Menu::Window.VisualsTab.OptionsWeapon.GetState())
				DrawInfo(pEntity, Box);

			if (Menu::Window.VisualsTab.OptionsAimSpot.GetState())
				DrawCross(pEntity);

			if (Menu::Window.VisualsTab.backtrackskele.GetState())
				drawBacktrackedSkelet(pEntity,pinfo);

			if (Menu::Window.VisualsTab.SightLines.GetState())
				SightLines(pEntity, Color);

			if (Menu::Window.VisualsTab.OptionsSkeleton.GetState())
				DrawSkeleton(pEntity);
		}
	}
}

void CEsp::SightLines(IClientEntity* pEntity, Color color)
{
	Vector src3D, dst3D, forward, src, dst;
	trace_t tr;
	Ray_t ray;
	CTraceFilter filter;
	IClientRenderable* pUnknoown = pEntity->GetClientRenderable();
	if (pUnknoown == nullptr)
		return;
	AngleVectors(pEntity->GetEyeAngles(), &forward);
	filter.pSkip = pEntity;
	src3D = pEntity->GetBonePos(6) - Vector(0, 0, 0);
	dst3D = src3D + (forward * Menu::Window.VisualsTab.SightLinesLength.GetValue());

	ray.Init(src3D, dst3D);
	tr.endpos = pEntity->GetEyeAngles();
	Interfaces::Trace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	if (!Render::WorldToScreen(src3D, src) || !Render::WorldToScreen(tr.endpos, dst))
		return;

	Render::Line(src.x, src.y, dst.x, dst.y, Color(255, 255, 255, 225));
	Render::DrawRect(dst.x - 3, dst.y - 3, 6, 6, Color(255, 255, 255, 255));
};

// Gets the 2D bounding box for the entity
// Returns false on failure nigga don't fail me
bool CEsp::GetBox(IClientEntity* pEntity, CEsp::ESPBox &result)
{
	// Variables
	Vector  vOrigin, min, max, sMin, sMax, sOrigin,
		flb, brt, blb, frt, frb, brb, blt, flt;
	float left, top, right, bottom;

	// Get the locations
	vOrigin = pEntity->GetOrigin();
	min = pEntity->collisionProperty()->GetMins() + vOrigin;
	max = pEntity->collisionProperty()->GetMaxs() + vOrigin;

	// Points of a 3d bounding box
	Vector points[] = { Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z) };

	// Get screen positions
	if (!Render::WorldToScreen(points[3], flb) || !Render::WorldToScreen(points[5], brt)
		|| !Render::WorldToScreen(points[0], blb) || !Render::WorldToScreen(points[4], frt)
		|| !Render::WorldToScreen(points[2], frb) || !Render::WorldToScreen(points[1], brb)
		|| !Render::WorldToScreen(points[6], blt) || !Render::WorldToScreen(points[7], flt))
		return false;

	// Put them in an array (maybe start them off in one later for speed?)
	Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

	// Init this shit
	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	// Find the bounding corners for our box
	for (int i = 1; i < 8; i++)
	{
		if (left > arr[i].x)
			left = arr[i].x;
		if (bottom < arr[i].y)
			bottom = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (top > arr[i].y)
			top = arr[i].y;
	}

	// Width / height
	result.x = left;
	result.y = top;
	result.w = right - left;
	result.h = bottom - top;

	return true;
}

// Get an entities color depending on team and vis ect
Color CEsp::GetPlayerColor(IClientEntity* pEntity)
{
	int TeamNum = pEntity->GetTeamNum();
	bool IsVis = GameUtils::IsVisible(hackManager.pLocal(), pEntity, (int)CSGOHitboxID::Head);

	Color color;

	if (TeamNum == TEAM_CS_T)
	{
		if (IsVis)
			color = Color(240, 240, 240, 240);
		else
			color = Color(240, 240, 240, 240);
	}
	else
	{
		if (IsVis)
			color = Color(240, 240, 240, 240);
		else
			color = Color(240, 240, 240, 240);
	}


	return color;
}

// 2D  Esp box
void CEsp::DrawBox(CEsp::ESPBox size, Color color)
{
	{
		{
			//bool IsVis = GameUtils::IsVisible(hackManager.pLocal(), pEntity, (int)CSGOHitboxID::Chest);  da dream
				{
					Render::Outline(size.x, size.y, size.w, size.h, color);
					Render::Outline(size.x - 1, size.y - 1, size.w + 1, size.h + 1, Color(0, 0, 0, 0));
					Render::Outline(size.x + 1, size.y + 1, size.w - 1, size.h - 1, Color(0, 0, 0, 0));
				}

			if (Menu::Window.VisualsTab.OptionsFilled.GetState())//this has to stay
			{
				Render::DrawRect(size.x, size.y, size.w, size.h, Color(20, 20, 20, 120));
			}
		}
	}
}

// Unicode Conversions
static wchar_t* CharToWideChar(const char* text)
{
	size_t size = strlen(text) + 1;
	wchar_t* wa = new wchar_t[size];
	mbstowcs_s(NULL, wa, size / 4, text, size);
	return wa;
}

// Player name
void CEsp::DrawName(player_info_t pinfo, CEsp::ESPBox size)
{
	if (strlen(pinfo.name) > 16)
	{
		pinfo.name[16] = 0;
		strcat(pinfo.name, "...");
		puts(pinfo.name);
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pinfo.name);
		Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y - 16, Color(255, 255, 255, 255), Render::Fonts::ESP, pinfo.name);
	}
	else
	{
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pinfo.name);
		Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y - 16, Color(255, 255, 255, 255), Render::Fonts::ESP, pinfo.name);
	}
}

// Draw a health bar. For Tf2 when a bar is bigger than max health a second bar is displayed
void CEsp::DrawHealth(IClientEntity* pEntity, CEsp::ESPBox size)
{
	int HPEnemy = 100;
	HPEnemy = pEntity->GetHealth();
	char nameBuffer[512];
	sprintf_s(nameBuffer, "%d", HPEnemy);


	float h = (size.h);
	float offset = (h / 4.f) + 5;
	float w = h / 48.f;
	float health = pEntity->GetHealth();
	UINT hp = h - (UINT)((h * health) / 100);

	int Red = 255 - (health*2.55);
	int Green = health*2.55;

	Render::DrawRect((size.x - 3) - 1, size.y - 1, 3, h + 2, Color(0, 0, 0, 180));

	Render::Line((size.x - 3), size.y + hp, (size.x - 3), size.y + h, Color(Red, Green, 0, 180));

	if (health < 100) {

		Render::Text(size.x - 9, size.y + hp, Color(255, 255, 255, 255), Render::Fonts::ESP, nameBuffer);
	}
}

// Cleans the internal class name up to something human readable and nice
std::string CleanItemName(std::string name)
{
	std::string Name = name;
	// Tidy up the weapon Name
	if (Name[0] == 'C')
		Name.erase(Name.begin());

	// Remove the word Weapon
	auto startOfWeap = Name.find("Weapon");
	if (startOfWeap != std::string::npos)
		Name.erase(Name.begin() + startOfWeap, Name.begin() + startOfWeap + 6);

	return Name;
}

// Anything else: weapons, class state? idk
void CEsp::DrawInfo(IClientEntity* pEntity, CEsp::ESPBox size)
{
	std::vector<std::string> Info;
	int i = 0;

	// Player Weapon ESP
	IClientEntity* pWeapon = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)pEntity->GetActiveWeaponHandle());
	if (Menu::Window.VisualsTab.OptionsWeapon.GetState() && pWeapon)
	{
		ClientClass* cClass = (ClientClass*)pWeapon->GetClientClass();
		if (cClass)
		{
			// Draw it
			Info.push_back(CleanItemName(cClass->m_pNetworkName));
		}
	}

	if (Menu::Window.VisualsTab.OptionsInfo.GetState() && pEntity == BombCarrier)
	{
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, "");

		Render::Text(size.x + size.w + 3, size.y + (i*(nameSize.bottom + 6) + 16), Color(255, 25, 25, 255), Render::Fonts::Menu, ("Bomb Carrier"));
		i++;
	}

	if (Menu::Window.VisualsTab.OptionsInfo.GetState() && pEntity->IsScoped())
	{
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, "");
		int i = 0;
		Render::Text(size.x + size.w + 3, size.y + (i*(nameSize.bottom + 4)), Color(0, 51, 102, 255), Render::Fonts::ESP, "ZOOM");
	}
	RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, "");
	char dmgmemes[64];
	sprintf_s(dmgmemes, "DMG: %.1f", autowalldmgtest[pEntity->GetIndex()]);
	Render::Text(size.x + size.w + 3, size.y + (i*(nameSize.bottom + 6)+10), Color(208, 202, 0, 255), Render::Fonts::ESP, dmgmemes);
	static RECT Size = Render::GetTextSize(Render::Fonts::Menu, "Text");
	for (auto Text : Info)
	{
		Render::Text(size.x + (size.w / 2) - (Size.right / 2), size.y + size.h + 2, Color(255, 255, 255, 255), Render::Fonts::Menu, Text.c_str());
		i++;
	}

}

void CEsp::drawBacktrackedSkelet(IClientEntity *base, player_info_t pinfo)
{
	//lagComp = new LagCompensation;

	int idx = base->getIdx();
	IClientEntity* pLocal = hackManager.pLocal();

	LagRecord *m_LagRecords = lagComp->m_LagRecord[idx];
	LagRecord recentLR = m_LagRecords[10];
	static int Scale = 2;
	Vector screenSpot;
	Vector screenSpot2;
	/*
	for (int i = 0; i < 9; i++)
	{

		if (Render::WorldToScreen(m_LagRecords[i].headSpot, screenSpot))
		{
			Render::DrawCircle(screenSpot.x, screenSpot.y, (2), (2 * 4), Color(0, 255, 25, 255));

		}
	}

	if (Render::WorldToScreen(recentLR.headSpot, screenSpot2))
	{
		Render::DrawCircle(screenSpot2.x, screenSpot2.y, (2), (2 * 4), Color(255, 25, 25, 255));

	}*/
	bool IsVis = GameUtils::IsVisible(hackManager.pLocal(), base, (int)CSGOHitboxID::Head);

	for (int i = 0; i < Interfaces::Engine->GetMaxClients(); i++)
	{
		if (Interfaces::Engine->GetPlayerInfo(i, &pinfo) && base->IsAlive())
		{
			if (Menu::Window.LegitBotTab.FakeLagFix.GetState() || Menu::Window.RageBotTab.FakeLagFix.GetState())
			{
					if (!IsVis)
						return;

					if (pLocal->IsAlive())
					{
						for (int t = 0; t < 12; ++t)
						{
							Vector screenbacktrack[64][12];

							if (headPositions[i][t].simtime && headPositions[i][t].simtime + 1 > pLocal->GetSimulationTime())
							{
								if (Render::WorldToScreen(headPositions[i][t].hitboxPos, screenbacktrack[i][t]))
								{

									Interfaces::Surface->DrawSetColor(Color::Green());
									Interfaces::Surface->DrawOutlinedRect(screenbacktrack[i][t].x, screenbacktrack[i][t].y, screenbacktrack[i][t].x + 2, screenbacktrack[i][t].y + 2);

								}
							}
						}
					}
					else
					{
						memset(&headPositions[0][0], 0, sizeof(headPositions));
					}
					if (pLocal->IsAlive())
					{
						for (int t = 0; t < 12; ++t)
						{
							Vector screenbacktrack[64][12];

							if (headPositions[i][t].simtime && headPositions[i][t].simtime + 1 > pLocal->GetSimulationTime())
							{
								if (Render::WorldToScreen(headPositions[i][t].hitboxPos, screenbacktrack[i][t]))
								{

									Interfaces::Surface->DrawSetColor(Color::Green());
									Interfaces::Surface->DrawOutlinedRect(screenbacktrack[i][t].x, screenbacktrack[i][t].y, screenbacktrack[i][t].x + 2, screenbacktrack[i][t].y + 2);

								}
							}
						}
					}
					else
					{
						memset(&headPositions[0][0], 0, sizeof(headPositions));
					}
				
			}
		}
	}

}


// Little cross on their heads
void CEsp::DrawCross(IClientEntity* pEntity)
{
	Vector cross = pEntity->GetHeadPos(), screen;
	static int Scale = 2;
	if (Render::WorldToScreen(cross, screen))
	{

		Render::Clear(screen.x - Scale, screen.y - (Scale * 2), (Scale * 2), (Scale * 4), Color(20, 20, 20, 160));
		Render::Clear(screen.x - (Scale * 2), screen.y - Scale, (Scale * 4), (Scale * 2), Color(20, 20, 20, 160));
		Render::Clear(screen.x - Scale - 1, screen.y - (Scale * 2) - 1, (Scale * 2) - 2, (Scale * 4) - 2, Color(250, 250, 250, 160));
		Render::Clear(screen.x - (Scale * 2) - 1, screen.y - Scale - 1, (Scale * 4) - 2, (Scale * 2) - 2, Color(250, 250, 250, 160));
	}
}

void CEsp::DefuseWarning(IClientEntity* pEntity)
{
	if (pEntity->IsDefusing())
		Render::Text(10, 100, Color(255, 25, 25, 255), Render::Fonts::ESP, ("Enemy is defusing"));
	else
		Render::Text(10, 100, Color(0, 255, 0, 255), Render::Fonts::ESP, (""));
}

// Draws a dropped CS:GO Item
void CEsp::DrawDrop(IClientEntity* pEntity, ClientClass* cClass)
{
	Vector Box;
	CBaseCombatWeapon* Weapon = (CBaseCombatWeapon*)pEntity;
	IClientEntity* plr = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)Weapon->GetOwnerHandle());
	if (!plr && Render::WorldToScreen(Weapon->GetOrigin(), Box))
	{
		if (Menu::Window.VisualsTab.FiltersWeapons.GetState())
		{
			std::string ItemName = CleanItemName(cClass->m_pNetworkName);
			RECT TextSize = Render::GetTextSize(Render::Fonts::ESP, ItemName.c_str());
			Render::Text(Box.x - (TextSize.right / 2), Box.y - 16, Color(255, 255, 255, 255), Render::Fonts::ESP, ItemName.c_str());
		}
	}
}

// Draw the planted bomb and timer
void CEsp::DrawBombPlanted(IClientEntity* pEntity, ClientClass* cClass)
{
	// Null it out incase bomb has been dropped or planted
	BombCarrier = nullptr;

	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	CCSBomb* Bomb = (CCSBomb*)pEntity;

	if (Render::WorldToScreen(vOrig, vScreen))
	{
		float flBlow = Bomb->GetC4BlowTime();
		float TimeRemaining = flBlow - (Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase());
		char buffer[64];
		if (TimeRemaining > 0)//happy now no more negative
		{
			sprintf_s(buffer, "[ Planted C4 %.1fs ]", TimeRemaining);
			Render::Text(vScreen.x, vScreen.y, Color(255, 25, 25, 255), Render::Fonts::ESP, buffer);
		}
		else {//this is a meme but i keep so the esp doesnt just dissappear once it explodes... makes it look cleaner
			sprintf_s(buffer, "[ Exploded C4 ]", TimeRemaining);
			Render::Text(vScreen.x, vScreen.y, Color(255, 25, 25, 255), Render::Fonts::ESP, buffer);
		}
	}
}

void CEsp::DrawBombPlanted2(IClientEntity* pEntity, ClientClass* cClass)
{
	// Null it out incase bomb has been dropped or planted
	BombCarrier = nullptr;

	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	CCSBomb* Bomb = (CCSBomb*)pEntity;

	float flBlow = Bomb->GetC4BlowTime();
	float TimeRemaining = flBlow - (Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase());
	char buffer[64];
	sprintf_s(buffer, "%.1fs Time To Defuse", TimeRemaining);
	if(hackManager.pLocal()->IsAlive())
		Render::Text(10, 400, Color(0, 255, 0, 255), Render::Fonts::ESP, buffer);
	else
		Render::Text(10, 350, Color(0, 255, 0, 255), Render::Fonts::ESP, buffer);

}

void CEsp::DrawBomb(IClientEntity* pEntity, ClientClass* cClass)
{
	// Null it out incase bomb has been dropped or planted
	BombCarrier = nullptr;
	CBaseCombatWeapon *BombWeapon = (CBaseCombatWeapon *)pEntity;
	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	bool adopted = true;
	HANDLE parent = BombWeapon->GetOwnerHandle();
	if (parent || (vOrig.x == 0 && vOrig.y == 0 && vOrig.z == 0))
	{
		IClientEntity* pParentEnt = (Interfaces::EntList->GetClientEntityFromHandle(parent));
		if (pParentEnt && pParentEnt->IsAlive())
		{
			BombCarrier = pParentEnt;
			adopted = false;
		}
	}

	if (adopted)
	{
		if (Render::WorldToScreen(vOrig, vScreen))
		{
			Render::Text(vScreen.x, vScreen.y, Color(255, 25, 25, 255), Render::Fonts::ESP, "[ Dropped C4 ]");
		}
	}
}

void DrawBoneArray(int* boneNumbers, int amount, IClientEntity* pEntity, Color color)
{
	Vector LastBoneScreen;
	for (int i = 0; i < amount; i++)
	{

		Vector Bone = pEntity->GetBonePos(boneNumbers[i]);
		Vector BoneScreen;

		if (Render::WorldToScreen(Bone, BoneScreen))
		{
			if (i>0)
			{
				Render::Line(LastBoneScreen.x, LastBoneScreen.y, BoneScreen.x, BoneScreen.y, color);
			}
		}
		LastBoneScreen = BoneScreen;
	}
}

void DrawBoneTest(IClientEntity *pEntity)
{
	for (int i = 0; i < 127; i++)
	{
		IClientRenderable* pUnknoown = pEntity->GetClientRenderable();
		if (pUnknoown == nullptr)
			return;
		Vector BoneLoc = pEntity->GetBonePos(i);
		Vector BoneScreen;
		if (Render::WorldToScreen(BoneLoc, BoneScreen))
		{
			char buf[10];
			_itoa_s(i, buf, 10);
			Render::Text(BoneScreen.x, BoneScreen.y, Color(255, 255, 255, 180), Render::Fonts::ESP, buf);
		}
	}
}

void CEsp::DrawSkeleton(IClientEntity* pEntity)
{
	studiohdr_t* pStudioHdr = Interfaces::ModelInfo->GetStudiomodel(pEntity->GetModel());
	 IClientEntity* pLocal = hackManager.pLocal();
	if (!pStudioHdr)
		return;

	Vector vParent, vChild, sParent, sChild;
	for (int j = 0; j < pStudioHdr->numbones; j++)
	{
		mstudiobone_t* pBone = pStudioHdr->GetBone(j);

		if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
		{
			IClientRenderable* pUnknoown = pEntity->GetClientRenderable();
			if (pUnknoown == nullptr)
				return;
			vChild = pEntity->GetBonePos(j);
			vParent = pEntity->GetBonePos(pBone->parent);

			if (Render::WorldToScreen(vParent, sParent) && Render::WorldToScreen(vChild, sChild))
			{
				Render::Line(sParent[0], sParent[1], sChild[0], sChild[1], Color(0, 0, 0, 255));
			}
		}
	}
}

void CEsp::BoxAndText(IClientEntity* entity, std::string text)
{
	ESPBox Box;
	std::vector<std::string> Info;
	if (GetBox(entity, Box))
	{
		Info.push_back(text);
			int i = 0;
			for (auto kek : Info)
			{
				Render::Text(Box.x + 1, Box.y + 1, Color(255, 255, 255, 255), Render::Fonts::ESP, kek.c_str());
				i++;
			}
	}
}