#pragma once
#include "BlurPipelineD3D9.h"
#include <unordered_map>
#include <memory>
#include <mutex>

// Pool manager for blur buffers, allowing multiple ImGui windows to use blur effects
class BlurBufferPool
{
public:
    BlurBufferPool() = default;
    ~BlurBufferPool();

    // Initialize the pool with the D3D9 device
    bool Initialize(IDirect3DDevice9* device);

    // Get or create a blur buffer for a specific window ID
    BlurPipelineD3D9* GetBuffer(unsigned int windowId);

    // Capture and blur a rectangle for a specific window
    IDirect3DTexture9* CaptureAndBlur(unsigned int windowId, const RECT& rect, 
                                      int iterations, float offset, int downsampleDiv);

    // Device lifecycle management
    void OnLostDevice();
    bool OnResetDevice(IDirect3DDevice9* device);

    // Cleanup all buffers
    void Cleanup();

private:
    IDirect3DDevice9* m_device = nullptr;
    std::unordered_map<unsigned int, std::unique_ptr<BlurPipelineD3D9>> m_buffers;
    std::mutex m_mutex;
    bool m_initialized = false;
};
