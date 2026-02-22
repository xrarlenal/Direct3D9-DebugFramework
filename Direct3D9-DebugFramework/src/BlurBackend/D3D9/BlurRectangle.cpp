#include "BlurRectangle.h"
#include "../../../imgui/imgui_internal.h"
#include <algorithm>
#include <cmath>

// 初始化静态计数器
unsigned int BlurRectangle::s_nextId = 0;

BlurRectangle::BlurRectangle(float width, float height, float rounding)
    : m_size(width, height)
    , m_position(100.0f, 100.0f)
    , m_rounding(rounding)
    , m_blurIterations(8)
    , m_downsampleDiv(2)
    , m_overlayBlackAlpha(0)
    , m_overlayWhiteAlpha(10)
    , m_contentBegun(false)
    , m_uniqueId(s_nextId++)
{
}

void BlurRectangle::SetSize(float width, float height)
{
    m_size = ImVec2(width, height);
}

void BlurRectangle::SetRounding(float rounding)
{
    m_rounding = rounding;
}

void BlurRectangle::SetPosition(float x, float y)
{
    m_position = ImVec2(x, y);
}

void BlurRectangle::SetBlurIterations(int iterations)
{
    m_blurIterations = std::clamp(iterations, MIN_BLUR_ITERATIONS, MAX_BLUR_ITERATIONS);
}

void BlurRectangle::SetDownsampleDiv(int div)
{
    m_downsampleDiv = std::clamp(div, MIN_DOWNSAMPLE_DIV, MAX_DOWNSAMPLE_DIV);
}

void BlurRectangle::SetOverlayBlackAlpha(int alpha)
{
    m_overlayBlackAlpha = std::clamp(alpha, 0, 255);
}

void BlurRectangle::SetOverlayWhiteAlpha(int alpha)
{
    m_overlayWhiteAlpha = std::clamp(alpha, 0, 255);
}

bool BlurRectangle::Render(IDirect3DDevice9* device, BlurPipelineD3D9& blurPipeline)
{
    if (!device)
        return false;

    // 目标矩形（你真正想显示的模糊区域）
    LONG tgtLeft   = (LONG)std::floor(m_position.x);
    LONG tgtTop    = (LONG)std::floor(m_position.y);
    LONG tgtRight  = (LONG)std::ceil(m_position.x + m_size.x);
    LONG tgtBottom = (LONG)std::ceil(m_position.y + m_size.y);

    if (tgtRight <= tgtLeft || tgtBottom <= tgtTop)
        return false;

    // 在目标矩形基础上向外扩一点做采样，避免边缘收缩
    const LONG border = 10; // 可以试 1~3，看效果

    RECT captureRect;
    captureRect.left   = tgtLeft   - border;
    captureRect.top    = tgtTop    - border;
    captureRect.right  = tgtRight  + border;
    captureRect.bottom = tgtBottom + border;

    // 防止越界：按当前 viewport clamp 一下
    D3DVIEWPORT9 vp{};
    if (SUCCEEDED(device->GetViewport(&vp)))
    {
        LONG vpLeft   = (LONG)vp.X;
        LONG vpTop    = (LONG)vp.Y;
        LONG vpRight  = (LONG)(vp.X + vp.Width);
        LONG vpBottom = (LONG)(vp.Y + vp.Height);

        if (captureRect.left   < vpLeft)   captureRect.left   = vpLeft;
        if (captureRect.top    < vpTop)    captureRect.top    = vpTop;
        if (captureRect.right  > vpRight)  captureRect.right  = vpRight;
        if (captureRect.bottom > vpBottom) captureRect.bottom = vpBottom;
    }

    if (captureRect.right <= captureRect.left ||
        captureRect.bottom <= captureRect.top)
    {
        return false;
    }

    blurPipeline.SetDownsampleDiv(m_downsampleDiv);
    IDirect3DTexture9* blurredTex =
        blurPipeline.CaptureAndBlurRect(captureRect, m_blurIterations, 2.0f);

    if (!blurredTex)
        return false;

    // 计算目标矩形在 captureRect 里的相对 UV
    const float texW = float(captureRect.right - captureRect.left);
    const float texH = float(captureRect.bottom - captureRect.top);

    ImVec2 uv0(
        (float(tgtLeft) - float(captureRect.left)) / texW,
        (float(tgtTop) - float(captureRect.top)) / texH
    );
    ImVec2 uv1(
        (float(tgtRight) - float(captureRect.left)) / texW,
        (float(tgtBottom) - float(captureRect.top)) / texH
    );

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // 画模糊背景：只显示 captureRect 中间这一块
    drawList->AddImageRounded(
        (ImTextureID)blurredTex,
        ImVec2((float)tgtLeft, (float)tgtTop),      // 直接构造 ImVec2
        ImVec2((float)tgtRight, (float)tgtBottom),
        uv0, uv1,
        IM_COL32(255, 255, 255, 255),
        m_rounding
    );

    if (m_overlayBlackAlpha > 0)
    {
        drawList->AddRectFilled(
            ImVec2((float)tgtLeft, (float)tgtTop),
            ImVec2((float)tgtRight, (float)tgtBottom),
            IM_COL32(0, 0, 0, m_overlayBlackAlpha),
            m_rounding
        );
    }

    if (m_overlayWhiteAlpha > 0)
    {
        drawList->AddRectFilled(
            ImVec2((float)tgtLeft, (float)tgtTop),
            ImVec2((float)tgtRight, (float)tgtBottom),
            IM_COL32(255, 255, 255, m_overlayWhiteAlpha),
            m_rounding
        );
    }

    drawList->AddRect(
        ImVec2((float)tgtLeft, (float)tgtTop),
        ImVec2((float)tgtRight, (float)tgtBottom),
        IM_COL32(255, 255, 255, 60),
        m_rounding,
        0,
        1.0f
    );
    return true;
}

void BlurRectangle::BeginContent()
{
    // 设置下一个窗口的位置和大小
    ImGui::SetNextWindowPos(m_position);
    ImGui::SetNextWindowSize(m_size);
    
    // 创建一个完全透明的无标题栏窗口用于内容
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground;
    
    // 设置窗口背景完全透明
    ImGui::SetNextWindowBgAlpha(0.0f);
    
    char windowName[64];
    snprintf(windowName, sizeof(windowName), "BlurRectContent_%u", m_uniqueId);
    
    if (ImGui::Begin(windowName, nullptr, flags))
    {
        m_contentBegun = true;
    }
}

void BlurRectangle::EndContent()
{
    if (m_contentBegun)
    {
        ImGui::End();
        m_contentBegun = false;
    }
}
