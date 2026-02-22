#include "main.h"



BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		hMainThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
		CreateThread(nullptr, 0, TestThread, hModule, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		d3d9_Hook::uninstallminhook();
		if (hMainThread) {CloseHandle(hMainThread);}
		break;
	}
	return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpParam)
{
	CreateDebugConsole();

	if (GetModuleHandleA("d3d9.dll"))
	{
		d3d9_Hook::installMinhook();
	}
	return 0;
}

DWORD WINAPI TestThread(LPVOID lpParam)
{
	for (;;)
	{
		Sleep(5000);
		std::cout << " Menu is " << ClickGUI::menuOpen << std::endl;
	}
}