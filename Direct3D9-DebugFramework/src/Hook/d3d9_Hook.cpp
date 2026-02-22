
#include "d3d9_Hook.h"
#include "../GUI/ClickGUI.h"

namespace d3d9_Hook
{
	EndScene_Template originalEndScene = nullptr;
	Reset_Template    originalReset = nullptr;
	std::atomic<bool> imguiInitialized(false);
	WNDPROC           originalWndProc = nullptr;
}

LPVOID d3d9_Hook::getHookAddress(int index)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0, 0,GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,L"DummyWndClass", nullptr };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindowW(wc.lpszClassName, L"Dummy", WS_OVERLAPPEDWINDOW,0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) {DestroyWindow(hWnd);UnregisterClass(wc.lpszClassName, wc.hInstance);MH_Uninitialize(); return nullptr;}

	D3DPRESENT_PARAMETERS d3dpp{};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	IDirect3DDevice9* device = nullptr;
	HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
	D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
	if (FAILED(hr) || !device) {d3d->Release();DestroyWindow(hWnd);UnregisterClass(wc.lpszClassName, wc.hInstance);MH_Uninitialize(); return nullptr;}


	void** vtable = *reinterpret_cast<void***>(device);


	void* addr = vtable[index];
	device->Release();
	d3d->Release();
	DestroyWindow(hWnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return addr;
}

void d3d9_Hook::installMinhook()
{
	if (MH_Initialize() != MH_OK) return;
	LPVOID EndSceneAddr = getHookAddress(42);
	LPVOID ResetAddr = getHookAddress(16);
	if (!EndSceneAddr or !ResetAddr) return;
	if (MH_CreateHook(EndSceneAddr, &hookEndScene, reinterpret_cast<LPVOID*>(&originalEndScene)) != MH_OK) return;
	if (MH_CreateHook(ResetAddr, &hookReset, reinterpret_cast<LPVOID*>(&originalReset)) != MH_OK) return;
	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return;
}

void d3d9_Hook::uninstallminhook()
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

HRESULT __stdcall d3d9_Hook::hookReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentParams)
{
	if (imguiInitialized.load())
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		HRESULT hr = originalReset(pDevice, pPresentParams);
		if (SUCCEEDED(hr))
			ImGui_ImplDX9_CreateDeviceObjects();
		return hr;
	}
	return originalReset(pDevice, pPresentParams);
}

HRESULT __stdcall d3d9_Hook::hookEndScene(IDirect3DDevice9* pDevice)
{
	if (!imguiInitialized.load())
	{
		HWND hwnd = nullptr;
		// 修复：声明 creation parameters 变量 cp（避免未定义标识符错误）
		D3DDEVICE_CREATION_PARAMETERS cp = {};
		if (pDevice && SUCCEEDED(pDevice->GetCreationParameters(&cp)))
			hwnd = cp.hFocusWindow;
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX9_Init(pDevice);
		originalWndProc = (WNDPROC)SetWindowLongPtrW(
		hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
		imguiInitialized.store(true);
	}
	ClickGUI::renderImguiD3D9(pDevice);   // 这里传 device
	return originalEndScene(pDevice);
}