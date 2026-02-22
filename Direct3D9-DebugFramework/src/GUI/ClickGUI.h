#pragma once
#include "../Hook/d3d9_Hook.h"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx9.h"

#include "../BlurBackend/D3D9/BlurRectangle.h"

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static BlurPipelineD3D9 g_MainBlurPipeline;
static bool             g_MainBlurInited = false;
static BlurRectangle    g_MainBlurRect(400.0f, 300.0f, 0.0f);

namespace { HHOOK g_keyboardHook = nullptr; }

namespace ClickGUI
{
	extern bool menuOpen;
	void renderImguiD3D9(IDirect3DDevice9* device);
}

