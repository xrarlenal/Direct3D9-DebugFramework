#pragma once
#include <d3d9.h>

class BlurPipelineD3D9
{
public:
    BlurPipelineD3D9() = default;
    ~BlurPipelineD3D9();

    bool Initialize(IDirect3DDevice9* device);
    void OnLostDevice();
    bool OnResetDevice(IDirect3DDevice9* device);

    IDirect3DTexture9* CaptureRect(const RECT& rect);
    IDirect3DTexture9* CaptureAndBlurRect(const RECT& rect, int iterations, float offset);

    HRESULT GetLastCaptureHr() const { return m_lastCaptureHr; }

    // NEW: 运行时可调参数
    void SetDownsampleDiv(int div) { m_downsampleDiv = div; }
    int  GetDownsampleDiv() const { return m_downsampleDiv; }

private:
    struct Level
    {
        int w = 0;
        int h = 0;
        IDirect3DTexture9* tex = nullptr;
        IDirect3DSurface9* surf = nullptr;
    };

    static constexpr int kMaxLevels = 8;

    bool EnsureLevels(int w, int h, int iterations);
    void DestroyLevels();

    HRESULT CaptureRectToTexture(const RECT& rect);

private:
    IDirect3DDevice9* m_device = nullptr;

    IDirect3DTexture9* m_captureTex = nullptr;
    IDirect3DSurface9* m_captureSurf = nullptr;
    int m_outputW = 0;
    int m_outputH = 0;

    Level m_level[kMaxLevels]{};
    int m_levels = 0;

    HRESULT m_lastCaptureHr = E_FAIL;

    // NEW: downsample 缩放倍数（2..8）
    int m_downsampleDiv = 4;
};