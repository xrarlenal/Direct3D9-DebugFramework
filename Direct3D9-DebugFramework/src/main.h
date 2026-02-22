#pragma once
#include <Windows.h>
#include <thread>
#include "GUI/ClickGUI.h"
#include "Hook/d3d9_Hook.h"

#ifdef DEBUG

#include "debug/debug.h"

#endif // DEBUG

DWORD WINAPI TestThread(LPVOID lpParam);
DWORD WINAPI MainThread(LPVOID lpParam); 
HANDLE hMainThread              =   nullptr;
HMODULE g_hModule = nullptr;

extern HMODULE g_hModule;