#include "ClickGUI.h"

bool ClickGUI::menuOpen = true;
extern HMODULE g_hModule;

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == VK_F2)
    {
        ClickGUI::menuOpen = !ClickGUI::menuOpen;
    }
    if (d3d9_Hook::imguiInitialized.load() && ClickGUI::menuOpen)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }
    return CallWindowProcW(d3d9_Hook::originalWndProc, hWnd, msg, wParam, lParam);
}


void ClickGUI::renderImguiD3D9(IDirect3DDevice9* device)
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    

    if (ClickGUI::menuOpen)
    {
        if (!g_MainBlurInited)
        {
            g_MainBlurPipeline.Initialize(device);

            g_MainBlurRect.SetBlurIterations(6);
            g_MainBlurRect.SetDownsampleDiv(2);
            g_MainBlurRect.SetOverlayBlackAlpha(80);
            g_MainBlurRect.SetOverlayWhiteAlpha(10);

            g_MainBlurInited = true;
        }
        ImGui::GetIO().MouseDrawCursor = true;
        {
            ImGui::Begin("Client Debug Window");
            ImGui::Button("Test Button");
            ImGui::End();
        }
        D3DVIEWPORT9 vp{};
        if (SUCCEEDED(device->GetViewport(&vp)))
        {
            float x = (float)vp.X;
            float y = (float)vp.Y;
            float width = (float)vp.Width;
            float height = (float)vp.Height;

            g_MainBlurRect.SetPosition(x, y);
            g_MainBlurRect.SetSize(width, height);

            if (g_MainBlurRect.Render(device, g_MainBlurPipeline))
            {
                g_MainBlurRect.BeginContent();
                g_MainBlurRect.EndContent();
            }
        }
    }
    else
    {
        ImGui::GetIO().MouseDrawCursor = false;
	}
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}