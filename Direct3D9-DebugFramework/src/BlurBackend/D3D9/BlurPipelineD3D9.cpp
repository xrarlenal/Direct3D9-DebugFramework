#include "BlurPipelineD3D9.h"
#include <algorithm>
#include <cstdio>

static void DebugHr(const char* tag, HRESULT hr)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "[Blur] %s hr=0x%08X\n", tag, (unsigned)hr);
    OutputDebugStringA(buf);
}

HRESULT BlurPipelineD3D9::CaptureRectToTexture(const RECT& rect)
{
    IDirect3DSurface9* back = nullptr;
    HRESULT hr = m_device->GetRenderTarget(0, &back);
    if (FAILED(hr) || !back)
    {
        m_lastCaptureHr = FAILED(hr) ? hr : E_FAIL;
        DebugHr("GetRenderTarget", m_lastCaptureHr);
        return m_lastCaptureHr;
    }

    hr = m_device->StretchRect(back, &rect, m_captureSurf, nullptr, D3DTEXF_LINEAR);
    m_lastCaptureHr = hr;
    DebugHr("StretchRect(back->capture)", hr);

    back->Release();
    return hr;
}

BlurPipelineD3D9::~BlurPipelineD3D9()
{
    OnLostDevice();
}

bool BlurPipelineD3D9::Initialize(IDirect3DDevice9* device)
{
    m_device = device;
    return m_device != nullptr;
}

void BlurPipelineD3D9::OnLostDevice()
{
    DestroyLevels();

    if (m_captureSurf) { m_captureSurf->Release(); m_captureSurf = nullptr; }
    if (m_captureTex) { m_captureTex->Release(); m_captureTex = nullptr; }
    m_outputW = m_outputH = 0;

    m_device = nullptr;
}

bool BlurPipelineD3D9::OnResetDevice(IDirect3DDevice9* device)
{
    m_device = device;
    return m_device != nullptr;
}

bool BlurPipelineD3D9::EnsureLevels(int w, int h, int iterations)
{
    if (!m_device) return false;

    // clamp iterations: 1..8（对 StretchRect pass 次数）
    iterations = std::clamp(iterations, 1, 8);

    // capture texture (full rect)
    if (!m_captureTex || m_outputW != w || m_outputH != h)
    {
        if (m_captureSurf) { m_captureSurf->Release(); m_captureSurf = nullptr; }
        if (m_captureTex) { m_captureTex->Release(); m_captureTex = nullptr; }

        m_outputW = w;
        m_outputH = h;

        if (FAILED(m_device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &m_captureTex, nullptr)))
            return false;

        if (FAILED(m_captureTex->GetSurfaceLevel(0, &m_captureSurf)))
            return false;
    }

    // 仍然只需要两个 level：down + full
    const int needLevels = 2;
    if (m_levels != needLevels)
    {
        DestroyLevels();
        m_levels = needLevels;
    }

    // downsample divisor: 2..8
    const int div = std::clamp(m_downsampleDiv, 2, 8);
    const int lw = (std::max)(1, (w + div - 1) / div);
    const int lh = (std::max)(1, (h + div - 1) / div);

    // level[0] (downsampled)
    {
        Level& lv = m_level[0];
        if (!(lv.tex && lv.surf && lv.w == lw && lv.h == lh))
        {
            if (lv.surf) { lv.surf->Release(); lv.surf = nullptr; }
            if (lv.tex) { lv.tex->Release(); lv.tex = nullptr; }
            lv.w = lw; lv.h = lh;

            if (FAILED(m_device->CreateTexture(lv.w, lv.h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &lv.tex, nullptr)))
                return false;

            if (FAILED(lv.tex->GetSurfaceLevel(0, &lv.surf)))
                return false;
        }
    }

    // level[1] (full output)
    {
        Level& lv = m_level[1];
        if (!(lv.tex && lv.surf && lv.w == w && lv.h == h))
        {
            if (lv.surf) { lv.surf->Release(); lv.surf = nullptr; }
            if (lv.tex) { lv.tex->Release(); lv.tex = nullptr; }
            lv.w = w; lv.h = h;

            if (FAILED(m_device->CreateTexture(lv.w, lv.h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &lv.tex, nullptr)))
                return false;

            if (FAILED(lv.tex->GetSurfaceLevel(0, &lv.surf)))
                return false;
        }
    }

    return true;
}

void BlurPipelineD3D9::DestroyLevels()
{
    for (int i = 0; i < kMaxLevels; ++i)
    {
        if (m_level[i].surf) { m_level[i].surf->Release(); m_level[i].surf = nullptr; }
        if (m_level[i].tex) { m_level[i].tex->Release(); m_level[i].tex = nullptr; }
        m_level[i].w = m_level[i].h = 0;
    }
    m_levels = 0;
}

IDirect3DTexture9* BlurPipelineD3D9::CaptureAndBlurRect(const RECT& rect, int iterations, float offset)
{
    (void)offset;

    if (!m_device) return nullptr;

    iterations = std::clamp(iterations, 1, 8);

    const int w = std::max<LONG>(1, rect.right - rect.left);
    const int h = std::max<LONG>(1, rect.bottom - rect.top);

    if (!EnsureLevels(w, h, iterations))
        return nullptr;

    IDirect3DSurface9* oldRT = nullptr;
    IDirect3DSurface9* oldDS = nullptr;
    D3DVIEWPORT9 oldVP{};
    HRESULT hr = S_OK;

    do
    {
        m_device->GetRenderTarget(0, &oldRT);
        m_device->GetDepthStencilSurface(&oldDS);
        m_device->GetViewport(&oldVP);

        hr = CaptureRectToTexture(rect);
        if (FAILED(hr))
            break;

        // NEW: 每轮基于“上一轮结果”继续模糊
        IDirect3DSurface9* srcSurf = m_captureSurf;
        for (int i = 0; i < iterations; ++i)
        {
            hr = m_device->StretchRect(srcSurf, nullptr, m_level[0].surf, nullptr, D3DTEXF_LINEAR);
            if (FAILED(hr)) break;

            hr = m_device->StretchRect(m_level[0].surf, nullptr, m_level[1].surf, nullptr, D3DTEXF_LINEAR);
            if (FAILED(hr)) break;

            // 下一轮从 level[1]（本轮输出）继续
            srcSurf = m_level[1].surf;
        }
    } while (false);

    if (oldRT) m_device->SetRenderTarget(0, oldRT);
    if (oldDS) m_device->SetDepthStencilSurface(oldDS);
    m_device->SetViewport(&oldVP);

    if (oldRT) oldRT->Release();
    if (oldDS) oldDS->Release();

    if (FAILED(hr))
        return nullptr;

    return m_level[1].tex;
}

IDirect3DTexture9* BlurPipelineD3D9::CaptureRect(const RECT& rect)
{
    if (!m_device) return nullptr;

    const int w = std::max<LONG>(1, rect.right - rect.left);
    const int h = std::max<LONG>(1, rect.bottom - rect.top);

    // 只确保 capture 纹理存在即可；levels/blur 不需要
    if (!m_captureTex || m_outputW != w || m_outputH != h)
    {
        if (m_captureSurf) { m_captureSurf->Release(); m_captureSurf = nullptr; }
        if (m_captureTex) { m_captureTex->Release(); m_captureTex = nullptr; }

        m_outputW = w;
        m_outputH = h;

        if (FAILED(m_device->CreateTexture(
            w, h, 1,
            D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT,
            &m_captureTex,
            nullptr)))
        {
            return nullptr;
        }

        if (FAILED(m_captureTex->GetSurfaceLevel(0, &m_captureSurf)))
            return nullptr;
    }

    // backup/restore 不变...
    IDirect3DSurface9* oldRT = nullptr;
    IDirect3DSurface9* oldDS = nullptr;
    D3DVIEWPORT9 oldVP{};
    m_device->GetRenderTarget(0, &oldRT);
    m_device->GetDepthStencilSurface(&oldDS);
    m_device->GetViewport(&oldVP);

    HRESULT hr = CaptureRectToTexture(rect);

    if (oldRT) m_device->SetRenderTarget(0, oldRT);
    if (oldDS) m_device->SetDepthStencilSurface(oldDS);
    m_device->SetViewport(&oldVP);

    if (oldRT) oldRT->Release();
    if (oldDS) oldDS->Release();

    if (FAILED(hr))
        return nullptr;

    return m_captureTex;
}