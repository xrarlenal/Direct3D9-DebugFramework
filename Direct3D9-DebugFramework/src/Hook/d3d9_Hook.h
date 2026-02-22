#include "../debug/debug.h"

#pragma once
#include <d3d9.h>
#include <Windows.h>
#include <atomic>


#include "../../minhook/MinHook.h"


#pragma comment(lib, "d3d9.lib")

#pragma comment(lib, "minhook/libMinHook.x86.lib")

namespace d3d9_Hook
{
	typedef HRESULT(__stdcall* EndScene_Template)(LPDIRECT3DDEVICE9 pDevice);
	typedef HRESULT(__stdcall* Reset_Template)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
	
	LPVOID getHookAddress(int index);
	HRESULT __stdcall hookEndScene(IDirect3DDevice9* pDevice);
	HRESULT __stdcall hookReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentParams);
	void installMinhook();
	void uninstallminhook();

	extern EndScene_Template             originalEndScene;
	extern Reset_Template                originalReset;
	extern std::atomic<bool>             imguiInitialized;
	extern WNDPROC                       originalWndProc;

}