#pragma once
#include <d3d9.h>
#include "../../../imgui/imgui.h"
#include "BlurPipelineD3D9.h"

// BlurRectangle: 无标题栏模糊圆角矩形类
// 提供简单优雅的接口，支持一行代码创建模糊圆角矩形
class BlurRectangle
{
public:
    // 常量定义
    static constexpr int MIN_BLUR_ITERATIONS = 1;
    static constexpr int MAX_BLUR_ITERATIONS = 8;
    static constexpr int MIN_DOWNSAMPLE_DIV = 2;
    static constexpr int MAX_DOWNSAMPLE_DIV = 8;
    
    // 构造函数：传入宽度、高度和圆角半径
    BlurRectangle(float width, float height, float rounding = 20.0f);
    
    // 设置矩形属性
    void SetSize(float width, float height);
    void SetRounding(float rounding);
    void SetPosition(float x, float y);
    
    // 获取矩形属性
    ImVec2 GetSize() const { return m_size; }
    ImVec2 GetPosition() const { return m_position; }
    float GetRounding() const { return m_rounding; }
    float GetHeight() const { return m_size.y; } // 新增：获取矩形高度
    float GetLength() const { return m_size.x; }

    // 设置模糊参数
    void SetBlurIterations(int iterations);
    void SetDownsampleDiv(int div);
    void SetOverlayBlackAlpha(int alpha);
    void SetOverlayWhiteAlpha(int alpha);
    
    // 渲染矩形
    // 返回是否成功渲染
    bool Render(IDirect3DDevice9* device, BlurPipelineD3D9& blurPipeline);
    
    // 开始内容区域（在调用Render后使用）
    void BeginContent();
    
    // 结束内容区域
    void EndContent();

private:
    ImVec2 m_size;              // 矩形大小
    ImVec2 m_position;          // 矩形位置
    float m_rounding;           // 圆角半径
    
    // 模糊参数
    int m_blurIterations;       // 模糊迭代次数
    int m_downsampleDiv;        // 降采样因子
    int m_overlayBlackAlpha;    // 黑色叠加透明度
    int m_overlayWhiteAlpha;    // 白色叠加透明度
    
    // 用于内容渲染的窗口ID
    bool m_contentBegun;
    unsigned int m_uniqueId;    // 唯一标识符
    
    // 静态计数器用于生成唯一ID
    static unsigned int s_nextId;
};
