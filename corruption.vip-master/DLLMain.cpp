#include "Utilities.h"
#include "INJ/ReflectiveLoader.h"
#include "Offsets.h"
#include "Interfaces.h"
#include "Hooks.h"
#include "RenderManager.h"
#include "Hacks.h"
#include "Menu.h"
#include "MiscHacks.h"
#include "Dumping.h"
#include "AntiAntiAim.h"
#include "hitmarker.h"
#include "lagcomp.h"

#include <stdio.h>
#include <string>
#include <iostream>

bool toggleSideSwitch;
extern HINSTANCE hAppInstance;
Vector AimPoint;
float intervalPerTick;
std::vector<int> HitBoxesToScan;
bool lbyupdate1;
float autowalldmgtest[65];
HINSTANCE HThisModule;
bool DoUnload;
bool pEntityLBYUpdate[65];
float pEntityLastUpdateTime[65];
float enemyLBYDelta[65];
int ResolverStage[65];
bool islbyupdate;
float ProxyLBYtime;
float lineLBY;
float lineRealAngle;
float lineFakeAngle; int LBYBreakerTimer;
bool rWeInFakeWalk;
float fsnLBY;
bool switchInverse;
float testFloat1;
float enemyLBYTimer[65];
float testFloat2;
int shotsfired[65];
float testFloat4;
int historyIdx = 10;
int hittedLogHits[65];
int missedLogHits[65];
float consoleProxyLbyLASTUpdateTime;
float enemysLastProxyTimer[65];

int InitialThread()
{
	Offsets::Initialise();
	Interfaces::Initialise();
	NetVar.RetrieveClasses();
	NetvarManager::Instance()->CreateDatabase();
	Render::Initialise();
	hitmarker::singleton()->initialize();
	Hacks::SetupHacks();
	Menu::SetupMenu();
	Hooks::Initialise();
	ApplyAAAHooks();
	lagComp = new LagCompensation;
	lagComp->initLagRecord();

	while (DoUnload == false)
	{
		Sleep(500);
	}

	Hooks::UndoHooks();
	Sleep(500);
	FreeLibraryAndExitThread(HThisModule, 0);

	return 0;

}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_QUERY_HMODULE:
		if (lpvReserved != NULL)
			*(HMODULE *)lpvReserved = hAppInstance;
		break;
	case DLL_PROCESS_ATTACH:
		HThisModule = hinstDLL;
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitialThread, NULL, NULL, NULL);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}