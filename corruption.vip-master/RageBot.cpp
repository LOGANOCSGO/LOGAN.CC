#include "RageBot.h"
#include "RenderManager.h"
#include "Resolver.h"
#include "Autowall.h"
#include "Hooks.h"
#include <iostream>
#include "UTIL Functions.h"
#include "lagcomp.h"
#include "EnginePrediction.h"

#define TIME_TO_TICKS( dt )	( ( int )( 0.5f + ( float )( dt ) / Interfaces::Globals->interval_per_tick ) )
#pragma warning(disable : 4020)
#define M_PI 3.14159265358979323846
#define	CONTENTS_EMPTY			0		// No contents

#define	CONTENTS_SOLID			0x1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x2		// translucent, but not watery (glass)
#define	CONTENTS_AUX			0x4
#define	CONTENTS_GRATE			0x8		// alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
#define	CONTENTS_SLIME			0x10
#define	CONTENTS_WATER			0x20
#define	CONTENTS_BLOCKLOS		0x40	// block AI line of sight
#define CONTENTS_OPAQUE			0x80	// things that cannot be seen through (may be non-solid though)
//#define	LAST_VISIBLE_CONTENTS	0x80

#define ALL_VISIBLE_CONTENTS (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

#define CONTENTS_TESTFOGVOLUME	0x100
#define CONTENTS_UNUSED			0x200	

// unused 
// NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
// if not visible, then grab from the bottom.
#define CONTENTS_UNUSED6		0x400

#define CONTENTS_TEAM1			0x800	// per team contents used to differentiate collisions 
#define CONTENTS_TEAM2			0x1000	// between players and objects on different teams

// ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
#define CONTENTS_IGNORE_NODRAW_OPAQUE	0x2000

// hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MOVEABLE		0x4000

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEBRIS			0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000
#define CONTENTS_HITBOX			0x40000000	// use accurate hitboxes on trace


// NOTE: These are stored in a short in the engine now.  Don't use more than 16 bits
#define	SURF_LIGHT		0x0001		// value will hold the light strength
#define	SURF_SKY2D		0x0002		// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS		0x0010
#define SURF_NOPORTAL	0x0020	// the surface can not have a portal placed on it
#define	SURF_TRIGGER	0x0040	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
#define	SURF_NODRAW		0x0080	// don't bother referencing the texture

#define	SURF_HINT		0x0100	// make a primary bsp splitter

#define	SURF_SKIP		0x0200	// completely ignore, allowing non-closed brushes
#define SURF_NOLIGHT	0x0400	// Don't calculate light
#define SURF_BUMPLIGHT	0x0800	// calculate three lightmaps for the surface for bumpmapping
#define SURF_NOSHADOWS	0x1000	// Don't receive shadows
#define SURF_NODECALS	0x2000	// Don't receive decals
#define SURF_NOCHOP		0x4000	// Don't subdivide patches on this surface 
#define SURF_HITBOX		0x8000	// surface is part of a hitbox



// -----------------------------------------------------
// spatial content masks - used for spatial queries (traceline,etc.)
// -----------------------------------------------------
#define	MASK_ALL					(0xFFFFFFFF)
// everything that is normally solid
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// everything that blocks player movement
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// blocks npc movement
#define	MASK_NPCSOLID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// water physics in these contents
#define	MASK_WATER					(CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)
// everything that blocks lighting
#define	MASK_OPAQUE					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)
// everything that blocks lighting, but with monsters added.
#define MASK_OPAQUE_AND_NPCS		(MASK_OPAQUE|CONTENTS_MONSTER)
// everything that blocks line of sight for AI
#define MASK_BLOCKLOS				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
// everything that blocks line of sight for AI plus NPCs
#define MASK_BLOCKLOS_AND_NPCS		(MASK_BLOCKLOS|CONTENTS_MONSTER)
// everything that blocks line of sight for players
#define	MASK_VISIBLE					(MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)
// everything that blocks line of sight for players, but with monsters added.
#define MASK_VISIBLE_AND_NPCS		(MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)
// bullets see these as solid
#define	MASK_SHOT					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
// non-raycasted weapons see this as solid (includes grates)
#define MASK_SHOT_HULL				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)
// hits solids (not grates) and passes through everything else
#define MASK_SHOT_PORTAL			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)
// everything normally solid, except monsters (world+brush only)
#define MASK_SOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)
// everything normally solid for player movement, except monsters (world+brush only)
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)
// everything normally solid for npc movement, except monsters (world+brush only)
#define MASK_NPCSOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// These are things that can split areaportals
#define MASK_SPLITAREAPORTAL		(CONTENTS_WATER|CONTENTS_SLIME)

// UNDONE: This is untested, any moving water
#define MASK_CURRENT				(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// everything that blocks corpse movement
// UNDONE: Not used yet / may be deleted
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE)
inline float RandomFloat(float min, float max)
{
	static auto fn = (decltype(&RandomFloat))(GetProcAddress(GetModuleHandle("vstdlib.dll"), "RandomFloat"));
	return fn(min, max);
}

inline void RandomSeed(int seed)
{
	static auto fn = (decltype(&RandomSeed))(GetProcAddress(GetModuleHandle("vstdlib.dll"), "RandomSeed"));

	return fn(seed);
}

#define XM_2PI              6.283185307f
#include <deque>
void CRageBot::Init()
{
	IsAimStepping = false;
	IsLocked = false;
	TargetID = -1;
}

void CRageBot::Draw()
{

}

inline bool CanAttack()
{
	IClientEntity* pLocalEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (pLocalEntity && pLocalEntity->IsAlive())
	{
		CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocalEntity->GetActiveWeaponHandle());
		if (pWeapon)
		{
			if (pWeapon->GetAmmoInClip() < 1 && GameUtils::IsBallisticWeapon(pWeapon))
				return false;

			bool revolver_can_shoot = true;
			if (pWeapon->m_AttributeManager()->m_Item()->GetItemDefinitionIndex() == 64)
			{
				float time_to_shoot = pWeapon->m_flPostponeFireReadyTime() - pLocalEntity->GetTickBase() * Interfaces::Globals->interval_per_tick;
				revolver_can_shoot = time_to_shoot <= Interfaces::Globals->absoluteframetime;
			}

			float time = pLocalEntity->GetTickBase() * Interfaces::Globals->interval_per_tick;
			float next_attack = pWeapon->GetNextPrimaryAttack();
			return revolver_can_shoot && !(next_attack > time);
		}
	}

	return false;
}



bool IsAbleToShoot(IClientEntity* pLocal)
{
	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());

	if (!pLocal)
		return false;

	if (!pWeapon)
		return false;

	float flServerTime = pLocal->GetTickBase() * Interfaces::Globals->interval_per_tick;

	return (!(pWeapon->GetNextPrimaryAttack() > flServerTime));
}

float hitchance(IClientEntity* pLocal, CBaseCombatWeapon* pWeapon)
{
	float hitchance = 75;
	if (!pWeapon) return 0;
	if (Menu::Window.RageBotTab.AimbotHitchance.GetValue() > 1)
	{
		float inaccuracy = pWeapon->GetInaccuracy();
		if (inaccuracy == 0) inaccuracy = 0.0000001;
		inaccuracy = 1 / inaccuracy;
		hitchance = inaccuracy;

	}
	return hitchance;
}
/*
bool hit_chance(IClientEntity* local, CUserCmd* cmd, CBaseCombatWeapon* weapon, IClientEntity* target)
{
	Vector forward, right, up;

	constexpr auto max_traces = 256;

	AngleVectors(cmd->viewangles, &forward, &right, &up);

	int total_hits = 0;
	int needed_hits = static_cast<int>(max_traces * (Menu::Window.RageBotTab.AimbotHitchance.GetValue() / 5000.f));

	weapon->UpdateAccuracyPenalty(weapon);

	auto eyes = local->GetEyePosition();
	auto flRange = weapon->GetCSWpnData()->flRange;

	for (int i = 0; i < max_traces; i++) {
		RandomSeed(i + 1);

		float fRand1 = RandomFloat(0.f, 1.f);
		float fRandPi1 = RandomFloat(0.f, XM_2PI);
		float fRand2 = RandomFloat(0.f, 1.f);
		float fRandPi2 = RandomFloat(0.f, XM_2PI);

		float fRandInaccuracy = fRand1 * weapon->GetInaccuracy();
		float fRandSpread = fRand2 * weapon->GetSpread();

		float fSpreadX = cos(fRandPi1) * fRandInaccuracy + cos(fRandPi2) * fRandSpread;
		float fSpreadY = sin(fRandPi1) * fRandInaccuracy + sin(fRandPi2) * fRandSpread;

		auto viewSpreadForward = (forward + fSpreadX * right + fSpreadY * up).Normalized();

		Vector viewAnglesSpread;
		VectorAngles(viewSpreadForward, viewAnglesSpread);
		GameUtils::NormaliseViewAngle(viewAnglesSpread);

		Vector viewForward;
		AngleVectors(viewAnglesSpread, &viewForward);
		viewForward.NormalizeInPlace();

		viewForward = eyes + (viewForward * flRange);

		trace_t tr;
		Ray_t ray;
		ray.Init(eyes, viewForward);

		Interfaces::Trace->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE, target, &tr);


		if (tr.m_pEnt == target)
			total_hits++;

		if (total_hits >= needed_hits)
			return true;

		if ((max_traces - i + total_hits) < needed_hits)
			return false;
	}

	return false;
}
*/
float InterpolationFix()
{
	static ConVar* cvar_cl_interp = Interfaces::CVar->FindVar("cl_interp");
	static ConVar* cvar_cl_updaterate = Interfaces::CVar->FindVar("cl_updaterate");
	static ConVar* cvar_sv_maxupdaterate = Interfaces::CVar->FindVar("sv_maxupdaterate");
	static ConVar* cvar_sv_minupdaterate = Interfaces::CVar->FindVar("sv_minupdaterate");
	static ConVar* cvar_cl_interp_ratio = Interfaces::CVar->FindVar("cl_interp_ratio");

	IClientEntity* pLocal = hackManager.pLocal();
	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(hackManager.pLocal()->GetActiveWeaponHandle());

	float cl_interp = cvar_cl_interp->GetFloat();
	int cl_updaterate = cvar_cl_updaterate->GetInt();
	int sv_maxupdaterate = cvar_sv_maxupdaterate->GetInt();
	int sv_minupdaterate = cvar_sv_minupdaterate->GetInt();
	int cl_interp_ratio = cvar_cl_interp_ratio->GetInt();

	if (sv_maxupdaterate <= cl_updaterate)
		cl_updaterate = sv_maxupdaterate;

	if (sv_minupdaterate > cl_updaterate)
		cl_updaterate = sv_minupdaterate;

	float new_interp = (float)cl_interp_ratio / (float)cl_updaterate;

	if (new_interp > cl_interp)
		cl_interp = new_interp;

	return max(cl_interp, cl_interp_ratio / cl_updaterate);
}

bool CanOpenFire()
{
	IClientEntity* pLocalEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (!pLocalEntity)
		return false;

	CBaseCombatWeapon* entwep = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocalEntity->GetActiveWeaponHandle());

	float flServerTime = (float)pLocalEntity->GetTickBase() * Interfaces::Globals->interval_per_tick;
	float flNextPrimaryAttack = entwep->GetNextPrimaryAttack();

	std::cout << flServerTime << " " << flNextPrimaryAttack << std::endl;

	return !(flNextPrimaryAttack > flServerTime);
}

void CRageBot::Move(CUserCmd *pCmd, bool &bSendPacket)
{
	IClientEntity* pLocalEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (!pLocalEntity)
		return;

	if (Menu::Window.LegitBotTab.AimbotEnable.GetState())
		return;

	if (pLocalEntity->IsAlive())
	{
		if (!Menu::Window.LegitBotTab.AimbotEnable.GetState())//actually we are going to make sure legitbot isnt on ;)
			DoAntiAim(pCmd, bSendPacket);//soo we are going to run this and as long as you dont have a pitch or anything chosen you're good?
		//predictionSystem->start(pCmd);//same exact fucking error vtable not found
		for (int i = 1; i < Interfaces::Engine->GetMaxClients(); ++i)
		{
			IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);

			if (!pEntity || !pLocalEntity) {
				continue;
			}

			if (pEntity->GetTeamNum() == pLocalEntity->GetTeamNum()) {
				continue;
			}
			if (pEntity->GetVelocity().Length2D() < 0.5f)
				continue;


			if (!pLocalEntity->IsAlive() || !pEntity->IsAlive() || pEntity->IsImmune()) {
				continue;
			}
			Global::oldSimulTime[i] = pLocalEntity->getSimulTime();

			/*if (Menu::Window.RageBotTab.FakeLagFix.GetState())
				lagComp->fakeLagFix(pEntity, historyIdx);*/
		}
		if (Menu::Window.RageBotTab.AimbotEnable.GetState())
			DoAimbot(pCmd, bSendPacket);
		for (int i = 1; i < Interfaces::Engine->GetMaxClients(); ++i)
		{
			IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);

			if (!pEntity || !pLocalEntity) {
				continue;
			}

			if (pEntity->GetTeamNum() == pLocalEntity->GetTeamNum()) {
				continue;
			}
			if (pEntity->GetVelocity().Length2D() < .5)
				continue;


			if (!pLocalEntity->IsAlive() || !pEntity->IsAlive() || pEntity->IsImmune()) {
				continue;
			}
			if (Menu::Window.RageBotTab.FakeLagFix.GetState())
			{
				//lagComp->setCurrentEnt(pEntity);
			}
		}
		//predictionSystem->end();
		//if (Menu::Window.RageBotTab.DisableInterp.GetState())
		//	pCmd->tick_count = TIME_TO_TICKS(InterpolationFix());//account for lerp time once disabling interpolation

		//if (Menu::Window.RageBotTab.AccuracyPositionAdjustment.GetState())
			//PositionAdjustment(pCmd);//


		if (Menu::Window.RageBotTab.AccuracyRecoil.GetState())
			DoNoRecoil(pCmd);
	}
	//LastAngle = pCmd->viewangles;
}



Vector BestPoint(IClientEntity *targetPlayer, Vector &final)
{
	IClientEntity* pLocal = hackManager.pLocal();
	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(hackManager.pLocal()->GetActiveWeaponHandle());
	int damage = pWeapon->GetCSWpnData()->iDamage;

	trace_t tr;
	Ray_t ray;
	CTraceFilter filter;

	filter.pSkip = targetPlayer;
	ray.Init(final + Vector(0, 0, 10), final);// 0,0,0 or 0,0,10? ///this should try to shoot at top of head?
	Interfaces::Trace->TraceRay(ray, MASK_SHOT, &filter, &tr);//idk if doing trace ray's is good?

	final = tr.endpos;
	return final;
}

void CRageBot::DoAimbot(CUserCmd *pCmd, bool &bSendPacket)
{
	IClientEntity* pTarget = nullptr;
	IClientEntity* pLocal = hackManager.pLocal();
	Vector Start = pLocal->GetViewOffset() + pLocal->GetOrigin();
	bool FindNewTarget = true;
	CSWeaponInfo* weapInfo = ((CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle()))->GetCSWpnData();
	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());
		if (!pWeapon || pWeapon->GetAmmoInClip() == 0 || GameUtils::IsBallisticWeapon(pWeapon) || GameUtils::IsG(pWeapon))
		{
			return;
		}

	if (IsLocked && TargetID >= 0 && HitBox >= 0)
	{
		pTarget = Interfaces::EntList->GetClientEntity(TargetID);
		if (pTarget  && TargetMeetsRequirements(pTarget))
		{
			HitBox = HitScan(pTarget);
			if (HitBox >= 0)
			{

				Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
				Vector View;
				Interfaces::Engine->GetViewAngles(View);
				float FoV = FovToPlayer(ViewOffset, View, pTarget, HitBox);
				if (FoV < Menu::Window.RageBotTab.AimbotFov.GetValue())
					FindNewTarget = false;
			}
		}
	}

	if (FindNewTarget)
	{
		Global::Shots = 0;
		TargetID = 0;
		pTarget = nullptr;
		HitBox = -1;

		switch (Menu::Window.RageBotTab.AimbotSelection.GetIndex())//removing because its ayyware meme...					// its fine to keep, its in skeet lmao //stilllll uselessssssss // SHUTUP NIGLET
		{
		case 0:
			//TargetID = GetTargetCrosshair();
			TargetID = GetTargetThreat(pCmd);
			break;
		case 1:
			TargetID = GetTargetDistance();
			break;
		case 2:
			TargetID = GetTargetHealth();
			break;
		}

		if (TargetID >= 0)
		{
			pTarget = Interfaces::EntList->GetClientEntity(TargetID);
		}
		else
		{
			pTarget = nullptr;
			HitBox = -1;
		}
	}
	if (pTarget == nullptr || TargetID < 0)
		FindNewTarget = true;

	Global::Target = pTarget;
	Global::TargetID = TargetID;

	if (TargetID >= 0 && pTarget)
	{
		HitBox = HitScan(pTarget);

		if (HitBox < 0)
			return;

		float pointscale = Menu::Window.RageBotTab.AimbotPointscale.GetValue() - 0.f;//aimheight... dont let it lie //tbh if we dont have this it aims at top of spine and its ghetto
		
		VMatrix BoneMatrix[128];
		Vector Point,point2;
		Vector point1;

		if(HitBox > 0)
			AimPoint = GetHitboxPosition(pTarget, HitBox);//if not head then we want to aim right at center
		else if(HitBox == 0)
			AimPoint = GetHitboxPosition(pTarget, HitBox) + Vector(0, 0, pointscale);//if is head use slider to adjust aimheight
		/*std::vector<Vector> Points = GetMultiplePointsForHitbox(pTarget, HitBox, BoneMatrix);
			for (int k = 0; k < Points.size(); k++) {
				point1 = Points.at(k);
			}
			QAngle aim = CalculateAngle(pLocal->GetEyeAngles(), point1);
			aim.y = NormalizeYaw(aim.y);*/
		pTarget->GetPredicted(AimPoint); /*Velocity Prediction*/

		if (Menu::Window.RageBotTab.AimbotMultipoint.GetState())
		{
			Point = BestPoint(pTarget, AimPoint);
		}
		else
		{
			Point = AimPoint;
		}
		bool can_shoot = (pWeapon->GetNextPrimaryAttack() <= (pLocal->GetTickBase() * Interfaces::Globals->interval_per_tick));

		if (can_shoot)
		{
			if (GameUtils::IsScopedWeapon(pWeapon) && !pWeapon->IsScoped() && Menu::Window.RageBotTab.AimbotAutoScope.GetState()) // Autoscope
			{
				pCmd->buttons |= IN_ATTACK2;
			}
			else
			{
				if ((Menu::Window.RageBotTab.AimbotHitchance.GetValue() * 1.5 <= hitchance(pLocal, pWeapon)) || Menu::Window.RageBotTab.AimbotHitchance.GetValue() == 0 || *pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() == 64)
				{
					//if (Menu::Window.RageBotTab.FakeLagFix.GetState())
						//pCmd->tick_count = lagComp->fixTickcount(pTarget);

					if (AimAtPoint(pLocal, Point, pCmd, bSendPacket))
					{
						if (Menu::Window.RageBotTab.AimbotAutoFire.GetState() && !(pCmd->buttons & IN_ATTACK))//check if sniper and scoped :P if you dont have autoscope on then your fucked because it wont shoot
						{
							shotsfired[TargetID] += 1;
							pCmd->buttons |= IN_ATTACK;
						}
						else
							return;
					}
					else if (Menu::Window.RageBotTab.AimbotAutoFire.GetState() && !(pCmd->buttons & IN_ATTACK))
					{
						shotsfired[TargetID]++;
						pCmd->buttons |= IN_ATTACK;
					}
				}
			}
		}
		if (pWeapon) {
			if (!(pCmd->buttons & IN_ATTACK) && pWeapon->GetNextPrimaryAttack() <= (pLocal->GetTickBase() * Interfaces::Globals->interval_per_tick)) {
				Global::Shots = 0;
				Global::missedshots = 0;
			}
			else {
				Global::Shots += pLocal->GetShotsFired();
			}
		}
		// Auto Stop
		if (TargetID >= 0 && pTarget)
		{
			if (Menu::Window.RageBotTab.AccuracyAutoStop.GetState())
			{
				pCmd->forwardmove = 0.f;
				pCmd->sidemove = 0.f;
			}
		}

		// Auto crouch + stop because wrapzii gay
		if (TargetID >= 0 && pTarget)
		{
			if (Menu::Window.RageBotTab.AccuracyAutoCrouch.GetState())
			{
				pCmd->forwardmove = 0.f;
				pCmd->sidemove = 0.f;
				pCmd->buttons |= IN_DUCK;
			}
		}

	}
	static bool WasFiring = false;
			if (GameUtils::IsPistol(pWeapon))
			{
				if (pCmd->buttons & IN_ATTACK)
				{
					if (WasFiring)
					{
						pCmd->buttons &= ~IN_ATTACK;
					}
				}
				WasFiring = pCmd->buttons & IN_ATTACK ? true : false;
			}
}

bool CRageBot::TargetMeetsRequirements(IClientEntity* pEntity)
{
	if (pEntity && pEntity->IsDormant() == false && pEntity->IsAlive() && pEntity->GetIndex() != hackManager.pLocal()->GetIndex())
	{
		ClientClass *pClientClass = pEntity->GetClientClass();
		player_info_t pinfo;
		if (pClientClass->m_ClassID == (int)CSGOClassID::CCSPlayer && Interfaces::Engine->GetPlayerInfo(pEntity->GetIndex(), &pinfo))
		{
			if (pEntity->GetTeamNum() != hackManager.pLocal()->GetTeamNum() || Menu::Window.RageBotTab.AimbotFriendlyFire.GetState())
			{
				if (!pEntity->HasGunGameImmunity())
				{
					return true;
				}
			}
		}
	}
	return false;
}

float CRageBot::FovToPlayer(Vector ViewOffSet, Vector View, IClientEntity* pEntity, int aHitBox)
{
	CONST FLOAT MaxDegrees = 180.0f;

	Vector Angles = View;

	Vector Origin = ViewOffSet;

	Vector Delta(0, 0, 0);

	Vector Forward(0, 0, 0);

	AngleVectors(Angles, &Forward);
	if (aHitBox >= 0) {
		Vector AimPos = GetHitboxPosition(pEntity, aHitBox);

		VectorSubtract(AimPos, Origin, Delta);

		Normalize(Delta, Delta);

		FLOAT DotProduct = Forward.Dot(Delta);

		return (acos(DotProduct) * (MaxDegrees / PI));
	}
}

int CRageBot::GetTargetCrosshair()
{
	int target = -1;
	float minFoV = Menu::Window.RageBotTab.AimbotFov.GetValue();

	IClientEntity* pLocal = hackManager.pLocal();
	Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
	Vector View; Interfaces::Engine->GetViewAngles(View);

	for (int i = 0; i < Interfaces::EntList->GetMaxEntities(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (TargetMeetsRequirements(pEntity))
		{
			int NewHitBox = HitScan(pEntity);
			if (NewHitBox >= 0)
			{
				float fov = FovToPlayer(ViewOffset, View, pEntity, 0);
				if (fov < minFoV)
				{
					minFoV = fov;
					target = i;
				}
			}
		}
	}

	return target;
}

int CRageBot::GetTargetDistance()
{
	int target = -1;
	int minDist = 99999;

	IClientEntity* pLocal = hackManager.pLocal();
	Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
	Vector View; Interfaces::Engine->GetViewAngles(View);

	for (int i = 0; i < Interfaces::EntList->GetMaxEntities(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (TargetMeetsRequirements(pEntity))
		{
			int NewHitBox = HitScan(pEntity);
			if (NewHitBox >= 0)
			{
				Vector Difference = pLocal->GetOrigin() - pEntity->GetOrigin();
				int Distance = Difference.Length();
				float fov = FovToPlayer(ViewOffset, View, pEntity, 0);
				if (Distance < minDist && fov < Menu::Window.RageBotTab.AimbotFov.GetValue())
				{
					minDist = Distance;
					target = i;
				}
			}
		}
	}

	return target;
}

int CRageBot::GetTargetNextShot()
{
	int target = -1;
	int minfov = 361;

	IClientEntity* pLocal = hackManager.pLocal();
	Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
	Vector View; Interfaces::Engine->GetViewAngles(View);

	for (int i = 0; i < Interfaces::EntList->GetMaxEntities(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (TargetMeetsRequirements(pEntity))
		{
			int NewHitBox = HitScan(pEntity);
			if (NewHitBox >= 0)
			{
				int Health = pEntity->GetHealth();
				float fov = FovToPlayer(ViewOffset, View, pEntity, 0);
				if (fov < minfov && fov < Menu::Window.RageBotTab.AimbotFov.GetValue())
				{
					minfov = fov;
					target = i;
				}
				else
					minfov = 361;
			}
		}
	}

	return target;
}

float GetFov(const QAngle& viewAngle, const QAngle& aimAngle)
{
	Vector ang, aim;

	AngleVectors(viewAngle, &aim);
	AngleVectors(aimAngle, &ang);

	return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSqr()));
}

double inline __declspec (naked) __fastcall FASTSQRT(double n)
{
	_asm fld qword ptr[esp + 4]
		_asm fsqrt
	_asm ret 8
}

float VectorDistance(Vector v1, Vector v2)
{
	return FASTSQRT(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2));
}

int CRageBot::GetTargetThreat(CUserCmd* pCmd)
{
	auto iBestTarget = -1;
	float flDistance = 8192.f;

	IClientEntity* pLocal = hackManager.pLocal();

	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
		Vector View; Interfaces::Engine->GetViewAngles(View);
		float minFOV = Menu::Window.RageBotTab.AimbotFov.GetValue();
		if (TargetMeetsRequirements(pEntity))
		{
			int NewHitBox = HitScan(pEntity);//tf we need to hitscan to tell where the enemies are?
			auto vecHitbox = pEntity->GetBonePos(0);//this was newhitbox
			if (NewHitBox >= 0)
			{
				Vector Difference = pLocal->GetOrigin() - pEntity->GetOrigin();
				QAngle TempTargetAbs;
				CalcAngle(pLocal->GetEyePosition(), vecHitbox, TempTargetAbs);
				float fov = FovToPlayer(ViewOffset,View, pEntity, 0);
				float flTempFOVs = GetFov(pCmd->viewangles, TempTargetAbs);
				float flTempDistance = VectorDistance(pLocal->GetOrigin(), pEntity->GetOrigin());
				if (flTempDistance < flDistance && fov < minFOV)//closest to crosshair and closest
				{
					minFOV = fov;
					flDistance = flTempDistance;
					iBestTarget = i;
				}
			}
		}
	}
	return iBestTarget;
}

int CRageBot::GetTargetHealth()
{
	int target = -1;
	int minHealth = 101;

	IClientEntity* pLocal = hackManager.pLocal();
	Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
	Vector View; Interfaces::Engine->GetViewAngles(View);

	for (int i = 0; i < Interfaces::EntList->GetMaxEntities(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (TargetMeetsRequirements(pEntity))
		{
			int NewHitBox = HitScan(pEntity);
			if (NewHitBox >= 0)
			{
				int Health = pEntity->GetHealth();
				float fov = FovToPlayer(ViewOffset, View, pEntity, 0);
				if (Health < minHealth && fov < Menu::Window.RageBotTab.AimbotFov.GetValue())
				{
					minHealth = Health;
					target = i;
				}
			}
		}
	}

	return target;
}

int CRageBot::HitScan(IClientEntity* pEntity)
{
	IClientEntity* pLocal = hackManager.pLocal();

#pragma region GetHitboxesToScan
	int HitScanMode = Menu::Window.RageBotTab.AimbotHitscan.GetIndex();
	int huso = pEntity->GetHealth();
	int health = Menu::Window.RageBotTab.BaimIfUnderXHealth.GetValue();
	bool AWall = Menu::Window.RageBotTab.AimbotAutoWall.GetState();
	bool Multipoint = Menu::Window.RageBotTab.AimbotMultipoint.GetState();
	int TargetHitbox = Menu::Window.RageBotTab.AimbotHitbox.GetIndex();
	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(hackManager.pLocal()->GetActiveWeaponHandle());

	HitBoxesToScan.clear();//should fix some hitbox shit

		if (huso < health)
		{
			HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
			HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
			HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
			HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
			HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
			HitBoxesToScan.push_back((int)CSGOHitboxID::LeftUpperArm);
			HitBoxesToScan.push_back((int)CSGOHitboxID::RightUpperArm);
			HitBoxesToScan.push_back((int)CSGOHitboxID::LeftThigh);
			HitBoxesToScan.push_back((int)CSGOHitboxID::RightThigh);
			HitBoxesToScan.push_back((int)CSGOHitboxID::LeftShin);
			HitBoxesToScan.push_back((int)CSGOHitboxID::RightShin);
			HitBoxesToScan.push_back((int)CSGOHitboxID::RightFoot);
			HitBoxesToScan.push_back((int)CSGOHitboxID::LeftFoot);
		}
		else {
			if (HitScanMode == 0)
			{
				switch (Menu::Window.RageBotTab.AimbotHitbox.GetIndex())
				{
				case 0:
					HitBoxesToScan.push_back((int)CSGOHitboxID::Head);
					break;
				case 1:
					HitBoxesToScan.push_back((int)CSGOHitboxID::Neck);
					HitBoxesToScan.push_back((int)CSGOHitboxID::NeckLower);
					break;
				case 2:
					HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
					break;
				case 3:
					HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					break;
				case 4:
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftFoot);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightFoot);
					break;
				}
			}
			else if (Menu::Window.RageBotTab.AWPAtBody.GetState() && GameUtils::AWP(pWeapon))
			{
				HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
				HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
				HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
				HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
			}

			else if (Menu::Window.RageBotTab.PreferBodyAim.GetState())
			{
				HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
				HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
				HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
				HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LeftUpperArm);
				HitBoxesToScan.push_back((int)CSGOHitboxID::RightUpperArm);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LeftThigh);
				HitBoxesToScan.push_back((int)CSGOHitboxID::RightThigh);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LeftShin);
				HitBoxesToScan.push_back((int)CSGOHitboxID::RightShin);
				HitBoxesToScan.push_back((int)CSGOHitboxID::RightFoot);
				HitBoxesToScan.push_back((int)CSGOHitboxID::LeftFoot);
			}/*
				else if (pEntity->GetVelocity().Length2D() < 36.f && pEntity->GetVelocity().Length2D() > 0.5f)
				{
					HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
				}*/
			else
			{
				switch (HitScanMode)
				{
				case 1:
					HitBoxesToScan.push_back((int)CSGOHitboxID::Head);
					HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					break;
				case 2://improved
					HitBoxesToScan.push_back((int)CSGOHitboxID::Head);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Neck);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftUpperArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightUpperArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
					HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Chest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftShin);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightShin);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftThigh);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightThigh);
					break;
				case 3:
					HitBoxesToScan.push_back((int)CSGOHitboxID::Head);
					HitBoxesToScan.push_back((int)CSGOHitboxID::NeckLower);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightFoot);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftFoot);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LowerChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Stomach);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Pelvis);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftLowerArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftUpperArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightLowerArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightUpperArm);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftThigh);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightThigh);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftShin);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightShin);
					HitBoxesToScan.push_back((int)CSGOHitboxID::Neck);
					HitBoxesToScan.push_back((int)CSGOHitboxID::UpperChest);
					HitBoxesToScan.push_back((int)CSGOHitboxID::RightHand);
					HitBoxesToScan.push_back((int)CSGOHitboxID::LeftHand);
					break;
				}
			}
		}
#pragma endregion Get the list of shit to scan
	int bestHitbox = -1;
	float highestDamage = Menu::Window.RageBotTab.AimbotMinimumDamage.GetValue();

	for (auto HitBoxID : HitBoxesToScan)
	{
		if (HitBoxID >= 0)
		{
			Vector Point = GetHitboxPosition(pEntity, HitBoxID);
			Vector Pointhd = GetHitboxPosition(pEntity, 0);
			float headdmg = 0.f;
			float Damage = 0.f;
			Color c = Color(255, 255, 255, 255);

			if (CanHit(pEntity, Point, &Damage))
			{
				/*if (headdmg > Damage && !Menu::Window.RageBotTab.AWPAtBody.GetState() && !Menu::Window.RageBotTab.PreferBodyAim.GetState())
					bestHitbox = 0;*/
				autowalldmgtest[pEntity->GetIndex()] = Damage;
				if ((Damage >= Menu::Window.RageBotTab.AimbotMinimumDamage.GetValue()) || (Menu::Window.RageBotTab.AimbotMinimumDamage.GetValue() > pEntity->GetHealth()))
				{
					return HitBoxID;
				}
			}
		}
	}
	return -1;
}

void CRageBot::DoNoRecoil(CUserCmd *pCmd)
{
	IClientEntity* pLocal = hackManager.pLocal();
	if (pLocal)
	{
		Vector AimPunch = pLocal->localPlayerExclusive()->GetAimPunchAngle();
		if (AimPunch.Length2D() > 0 && AimPunch.Length2D() < 150)
		{
			pCmd->viewangles -= AimPunch * 2; //AimPunch.x Aimpunch.y Aimpunch.z
			GameUtils::NormaliseViewAngle(pCmd->viewangles);
		}
	}
}

void CRageBot::aimAtPlayer(CUserCmd *pCmd)
{
	IClientEntity* pLocal = hackManager.pLocal();

	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(hackManager.pLocal()->GetActiveWeaponHandle());

	if (!pLocal || !pWeapon)
		return;

	Vector eye_position = pLocal->GetEyePosition();

	float best_dist = pWeapon->GetCSWpnData()->flRange;
	
	IClientEntity* target = nullptr;

	for (int i = 0; i <= Interfaces::Engine->GetMaxClients(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (TargetMeetsRequirements(pEntity))
		{
			if (Global::TargetID != -1)
				target = Interfaces::EntList->GetClientEntity(Global::TargetID);
			else
				target = pEntity;

			Vector target_position = target->GetEyePosition();

			float temp_dist = eye_position.DistTo(target_position);

			if (best_dist > temp_dist)
			{
				best_dist = temp_dist;
				CalcAngle(eye_position, target_position, pCmd->viewangles);
			}
		}
	}
}

bool CRageBot::AimAtPoint(IClientEntity* pLocal, Vector point, CUserCmd *pCmd, bool &bSendPacket)
{
	bool ReturnValue = false;

	if (point.Length() == 0) return ReturnValue;

	Vector angles;
	Vector src = pLocal->GetOrigin() + pLocal->GetViewOffset();

	CalcAngle(src, point, angles);
	GameUtils::NormaliseViewAngle(angles);

	if (angles[0] != angles[0] || angles[1] != angles[1])
	{
		return ReturnValue;
	}

	IsLocked = true;

	Vector ViewOffset = pLocal->GetOrigin() + pLocal->GetViewOffset();
	if (!IsAimStepping)
		LastAimstepAngle = LastAngle;

	float fovLeft = FovToPlayer(ViewOffset, LastAimstepAngle, Interfaces::EntList->GetClientEntity(TargetID), 0);

	if (fovLeft > 25.0f && Menu::Window.RageBotTab.AimbotAimStep.GetState())
	{

		Vector AddAngs = angles - LastAimstepAngle;
		Normalize(AddAngs, AddAngs);
		AddAngs *= 25;
		LastAimstepAngle += AddAngs;
		GameUtils::NormaliseViewAngle(LastAimstepAngle);
		angles = LastAimstepAngle;
	}
	else
	{
		ReturnValue = true;
	}

	if (Menu::Window.RageBotTab.AimbotSilentAim.GetState())
	{
		pCmd->viewangles = angles;

	}

	if (!Menu::Window.RageBotTab.AimbotSilentAim.GetState())
	{

		Interfaces::Engine->SetViewAngles(angles);
	}

	return ReturnValue;
}

namespace AntiAims
{
	void JitterPitch(CUserCmd *pCmd)
	{
		static bool up = true;
		if (up)
		{
			pCmd->viewangles.x = 45;
			up = !up;
		}
		else
		{
			pCmd->viewangles.x = 89;
			up = !up;
		}
	}

	void FakePitch(CUserCmd *pCmd, bool &bSendPacket)
	{
		static int ChokedPackets = -1;
		ChokedPackets++;
		if (ChokedPackets < 1)
		{
			bSendPacket = false;
			pCmd->viewangles.x = 89;
		}
		else
		{
			bSendPacket = true;
			pCmd->viewangles.x = 51;
			ChokedPackets = -1;
		}
	}

	void StaticJitter(CUserCmd *pCmd)
	{
		static bool down = true;
		if (down)
		{
			pCmd->viewangles.x = 179.0f;
			down = !down;
		}
		else
		{
			pCmd->viewangles.x = 89.0f;
			down = !down;
		}
	}

	// Yaws

	void FastSpin(CUserCmd *pCmd)
	{
		static int y2 = -179;
		int spinBotSpeedFast = 100;

		y2 += spinBotSpeedFast;

		if (y2 >= 179)
			y2 = -179;

		pCmd->viewangles.y = y2;
	}



	void FastSpint(CUserCmd *pCmd)
	{
		int r1 = rand() % 100;
		int r2 = rand() % 1000;

		static bool dir;
		static float current_y = pCmd->viewangles.y;

		if (r1 == 1) dir = !dir;

		if (dir)
			current_y += 15 + rand() % 10;
		else
			current_y -= 15 + rand() % 10;

		pCmd->viewangles.y = current_y;

		if (r1 == r2)
			pCmd->viewangles.y += r1;
	}

	void Up(CUserCmd *pCmd)
	{
		pCmd->viewangles.x = -89.0f;
	}

	void Zero(CUserCmd *pCmd)
	{
		pCmd->viewangles.x = 0.f;
	}

	void StaticRealYaw(CUserCmd *pCmd, bool &bSendPacket) {
		bSendPacket = false;
		pCmd->viewangles.y += 90;
	}

	void SideJitterFake(CUserCmd *pCmd)
	{
		static bool Fast2 = false;
		if (Fast2)
		{
			pCmd->viewangles.y -= 75;
		}
		else
		{
			pCmd->viewangles.y -= 105;
		}
		Fast2 = !Fast2;
	}

	void SideJitter(CUserCmd *pCmd)
	{
		static bool Fast2 = false;
		if (Fast2)
		{
			pCmd->viewangles.y += 75;
		}
		else
		{
			pCmd->viewangles.y += 105;
		}
		Fast2 = !Fast2;
	}

	void SlowSpin(CUserCmd *pCmd)
	{
		int r1 = rand() % 100;
		int r2 = rand() % 1000;

		static bool dir;
		static float current_y = pCmd->viewangles.y;

		if (r1 == 1) dir = !dir;

		if (dir)
			current_y += 4 + rand() % 10;
		else
			current_y -= 4 + rand() % 10;

		pCmd->viewangles.y = current_y;

		if (r1 == r2)
			pCmd->viewangles.y += r1;
	}
}

void CorrectMovement(Vector old_angles, CUserCmd* cmd, float old_forwardmove, float old_sidemove)
{
	float delta_view, first_function, second_function;

	if (old_angles.y < 0.f) first_function = 360.0f + old_angles.y;
	else first_function = old_angles.y;
	if (cmd->viewangles.y < 0.0f) second_function = 360.0f + cmd->viewangles.y;
	else second_function = cmd->viewangles.y;

	if (second_function < first_function) delta_view = abs(second_function - first_function);
	else delta_view = 360.0f - abs(first_function - second_function);

	delta_view = 360.0f - delta_view;

	cmd->forwardmove = cos(DEG2RAD(delta_view)) * old_forwardmove + cos(DEG2RAD(delta_view + 90.f)) * old_sidemove;
	cmd->sidemove = sin(DEG2RAD(delta_view)) * old_forwardmove + sin(DEG2RAD(delta_view + 90.f)) * old_sidemove;
}

float GetLatency()
{
	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	if (nci)
	{

		float Latency = nci->GetAvgLatency(FLOW_OUTGOING) + nci->GetAvgLatency(FLOW_INCOMING);
		return Latency;
	}
	else
	{

		return 0.0f;
	}
}
float GetOutgoingLatency()
{
	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	if (nci)
	{

		float OutgoingLatency = nci->GetAvgLatency(FLOW_OUTGOING);
		return OutgoingLatency;
	}
	else
	{

		return 0.0f;
	}
}
float GetIncomingLatency()
{
	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	if (nci)
	{
		float IncomingLatency = nci->GetAvgLatency(FLOW_INCOMING);
		return IncomingLatency;
	}
	else
	{

		return 0.0f;
	}
}


float OldLBY;
float LastLBYUpdateTime;
bool bSwitch;
float CurrentVelocity(IClientEntity* LocalPlayer)
{
	int vel = LocalPlayer->GetVelocity().Length2D();
	return vel;
}
bool NextLBYUpdate()
{
	IClientEntity* LocalPlayer = hackManager.pLocal();

	float flServerTime = (float)(LocalPlayer->GetTickBase()  * Interfaces::Globals->interval_per_tick);


	if (OldLBY != LocalPlayer->GetLowerBodyYaw())
	{

		LBYBreakerTimer++;
		OldLBY = LocalPlayer->GetLowerBodyYaw();
		bSwitch = !bSwitch;
		LastLBYUpdateTime = flServerTime;
	}

	if (CurrentVelocity(LocalPlayer) > 0.5)
	{
		LastLBYUpdateTime = flServerTime;
		return false;
	}

	if ((LastLBYUpdateTime + 1 - (GetLatency() * 2) < flServerTime) && (LocalPlayer->GetFlags() & FL_ONGROUND))
	{
		if (LastLBYUpdateTime + 1.1 - (GetLatency() * 2) < flServerTime)
		{
			LastLBYUpdateTime += 1.1;
		}
		return true;
	}
	return false;
}

void SideJitterALT(CUserCmd * pCmd, IClientEntity* pLocal, bool& bSendPacket)
{
	if (!bSendPacket)
	{
		static bool Fast2 = false;
		if (Fast2)
		{
			pCmd->viewangles.y += 75;
		}
		else
		{
			pCmd->viewangles.y += 105;
		}
		Fast2 = !Fast2;
	}
	else
	{
		pCmd->viewangles.y += 90;
	}
}

void SideJitter(CUserCmd * pCmd, IClientEntity* pLocal, bool& bSendPacket)
{
	if (!bSendPacket)
	{
		static bool Fast2 = false;
		if (Fast2)
		{
			pCmd->viewangles.y -= 75;
		}
		else
		{
			pCmd->viewangles.y -= 105;
		}
		Fast2 = !Fast2;
	}
	else
	{
		pCmd->viewangles.y -= 90;
	}
}


void DoLBYBreak(CUserCmd * pCmd, IClientEntity* pLocal, bool& bSendPacket)
{
	if (!bSendPacket)
	{
		pCmd->viewangles.y -= 90;
	}
	else
	{
		pCmd->viewangles.y += 90;
	}
}

void DoLBYBreakReal(CUserCmd * pCmd, IClientEntity* pLocal, bool& bSendPacket)
{
	if (!bSendPacket)
	{
		pCmd->viewangles.y += 90;
	}
	else
	{
		pCmd->viewangles.y -= 90;
	}
}
static bool peja;
static bool switchbrak;
static bool safdsfs;


void DoRealAA(CUserCmd* pCmd, IClientEntity* pLocal, bool& bSendPacket)
{

	static bool switch2;
	Vector oldAngle = pCmd->viewangles;
	float oldForward = pCmd->forwardmove;
	float oldSideMove = pCmd->sidemove;
	if (!Menu::Window.RageBotTab.AntiAimEnable.GetState())
		return;

	if (Menu::Window.RageBotTab.BreakLBY.GetIndex() == 0)
	{
		//nothing nigger
	}

	if (Menu::Window.RageBotTab.BreakLBY.GetIndex() == 1)
	{
		if (NextLBYUpdate())
		{
			//	if (side == AAYaw::ADAPTIVE_LEFT) //prob what u are here for, ignore the adaptive shit, it was for testing. hf
			pCmd->viewangles.y += 45;
			//	else if (side == AAYaw::ADAPTIVE_RIGHT)
			pCmd->viewangles.y -= 45;
			//else
			{
				pCmd->viewangles.y += 45;
			}
		}
	}

	if (Menu::Window.RageBotTab.BreakLBY.GetIndex() == 2)
	{
		if (NextLBYUpdate())
		{
			//	if (side == AAYaw::ADAPTIVE_LEFT) //prob what u are here for, ignore the adaptive shit, it was for testing. hf
			pCmd->viewangles.y += 90;
			//	else if (side == AAYaw::ADAPTIVE_RIGHT)
			pCmd->viewangles.y -= 90;
			//else
			{
				pCmd->viewangles.y += 90;
			}
		}
	}

	if (Menu::Window.RageBotTab.BreakLBY.GetIndex() == 3)
	{
		if (NextLBYUpdate())
		{
			//	if (side == AAYaw::ADAPTIVE_LEFT) //prob what u are here for, ignore the adaptive shit, it was for testing. hf
			pCmd->viewangles.y += 180;
			//	else if (side == AAYaw::ADAPTIVE_RIGHT)
			pCmd->viewangles.y -= 180;
			//else
			{
				pCmd->viewangles.y += 180;
			}
		}
	}


	switch (Menu::Window.RageBotTab.AntiAimYaw.GetIndex())
	{
	case 0:
		break;
	case 1:
		//backwards
		pCmd->viewangles.y -= 180;
		break;
	case 2:
		AntiAims::StaticRealYaw(pCmd, bSendPacket);
		break;
	case 3:
		AntiAims::SideJitter(pCmd);
	}


	if (toggleSideSwitch)
		pCmd->viewangles.y += Menu::Window.RageBotTab.AntiAimOffset.GetValue() + 180;
	else
		pCmd->viewangles.y += Menu::Window.RageBotTab.AntiAimOffset.GetValue();
}

void DoFakeAA(CUserCmd* pCmd, bool& bSendPacket, IClientEntity* pLocal)
{

	static bool switch2;
	Vector oldAngle = pCmd->viewangles;
	float oldForward = pCmd->forwardmove;
	float oldSideMove = pCmd->sidemove;
	if (!Menu::Window.RageBotTab.AntiAimEnable.GetState())
		return;
	switch (Menu::Window.RageBotTab.FakeYaw.GetIndex())
	{
	case 0:
		break;
	case 1:
		// Fast Spin 
		AntiAims::FastSpint(pCmd);
		break;
	case 2:
		// Slow Spin 
		AntiAims::SlowSpin(pCmd);
		break;
	case 3:
		AntiAims::SideJitterFake(pCmd);
		break;
	}


	if (toggleSideSwitch)
		pCmd->viewangles.y += Menu::Window.RageBotTab.AddFakeYaw.GetValue() + 180;
	else
		pCmd->viewangles.y += Menu::Window.RageBotTab.AddFakeYaw.GetValue();

}

void CRageBot::DoAntiAim(CUserCmd *pCmd, bool &bSendPacket)
{
	IClientEntity* pLocal = hackManager.pLocal();
	static int ChokedPackets = -1;
	if ((pCmd->buttons & IN_USE) || pLocal->GetMoveType() == MOVETYPE_LADDER)
		return;

	if ((IsAimStepping || pCmd->buttons & IN_ATTACK))
		return;

	CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(hackManager.pLocal()->GetActiveWeaponHandle());
	if (pWeapon)
	{
		CSWeaponInfo* pWeaponInfo = pWeapon->GetCSWpnData();
		// Knives or grenades
		CCSGrenade* csGrenade = (CCSGrenade*)pWeapon;

		if (GameUtils::IsBallisticWeapon(pWeapon))
		{
			if (Menu::Window.RageBotTab.AntiAimKnife.GetState())
			{
				if (!CanOpenFire() || pCmd->buttons & IN_ATTACK2)
					return;
			}
			else
			{
				if (csGrenade->GetThrowTime() > 0.f)
					return;
			}
		}
	}
	if (Menu::Window.RageBotTab.AntiAimTarget.GetState())
	{
		aimAtPlayer(pCmd);
	}

	switch (Menu::Window.RageBotTab.AntiAimPitch.GetIndex())
	{
	case 0:
		break;
	case 1:
		pCmd->viewangles.x = 45.f;
		break;
	case 2:
		AntiAims::JitterPitch(pCmd);
		break;
	case 3:
		pCmd->viewangles.x = 89.000000;
		break;
	case 4:
		AntiAims::Up(pCmd);
		break;
	case 5:
		AntiAims::Zero(pCmd);
		break;

	}

	if (Menu::Window.RageBotTab.AntiAimEnable.GetState())
	{
		static int ChokedPackets = -1;
		ChokedPackets++;
		if (ChokedPackets < 1)
		{

			bSendPacket = true;
			DoFakeAA(pCmd, bSendPacket, pLocal);
		}
		else
		{

			bSendPacket = false;
			DoRealAA(pCmd, pLocal, bSendPacket);
			ChokedPackets = -1;
		}

		if (flipAA)
		{
			pCmd->viewangles.y -= 25;
		}
	}

}
