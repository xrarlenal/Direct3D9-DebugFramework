#include "BlurBufferPool.h"
#include <algorithm>

BlurBufferPool::~BlurBufferPool()
{
    Cleanup();
}

bool BlurBufferPool::Initialize(IDirect3DDevice9* device)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!device)
        return false;
    
    m_device = device;
    m_initialized = true;
    return true;
}

BlurPipelineD3D9* BlurBufferPool::GetBuffer(unsigned int windowId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || !m_device)
        return nullptr;
    
    // Check if buffer already exists for this window
    auto it = m_buffers.find(windowId);
    if (it != m_buffers.end())
    {
        return it->second.get();
    }
    
    // Create new buffer for this window
    auto newBuffer = std::make_unique<BlurPipelineD3D9>();
    if (!newBuffer->Initialize(m_device))
    {
        return nullptr;
    }
    
    BlurPipelineD3D9* bufferPtr = newBuffer.get();
    m_buffers[windowId] = std::move(newBuffer);
    
    return bufferPtr;
}

IDirect3DTexture9* BlurBufferPool::CaptureAndBlur(unsigned int windowId, const RECT& rect,
                                                   int iterations, float offset, int downsampleDiv)
{
    // Note: This method duplicates buffer creation logic from GetBuffer() to maintain
    // atomicity of the entire capture-and-blur operation under a single mutex lock.
    // Calling GetBuffer() separately would release and reacquire the lock between
    // buffer retrieval and blur operations, potentially causing race conditions.
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized || !m_device)
        return nullptr;
    
    // Check if buffer already exists for this window
    auto it = m_buffers.find(windowId);
    BlurPipelineD3D9* buffer = nullptr;
    
    if (it != m_buffers.end())
    {
        buffer = it->second.get();
    }
    else
    {
        // Create new buffer for this window
        auto newBuffer = std::make_unique<BlurPipelineD3D9>();
        if (!newBuffer->Initialize(m_device))
        {
            return nullptr;
        }
        
        buffer = newBuffer.get();
        m_buffers[windowId] = std::move(newBuffer);
    }
    
    buffer->SetDownsampleDiv(downsampleDiv);
    return buffer->CaptureAndBlurRect(rect, iterations, offset);
}

void BlurBufferPool::OnLostDevice()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& pair : m_buffers)
    {
        pair.second->OnLostDevice();
    }
    
    m_device = nullptr;
}

bool BlurBufferPool::OnResetDevice(IDirect3DDevice9* device)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!device)
        return false;
    
    m_device = device;
    
    bool allSuccess = true;
    for (auto& pair : m_buffers)
    {
        if (!pair.second->OnResetDevice(device))
        {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

void BlurBufferPool::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_buffers.clear();
    m_device = nullptr;
    m_initialized = false;
}
