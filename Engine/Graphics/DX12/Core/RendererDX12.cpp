#include "RendererDX12.h"
#include <windows.h>

RendererDX12::RendererDX12()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
}

RendererDX12::~RendererDX12()
{
}

void RendererDX12::Initialize(void* windowHandle, int width, int height)
{
    HWND hwnd = static_cast<HWND>(windowHandle);

    m_deviceResources->SetWindow(hwnd, width, height);
    m_deviceResources->CreateDeviceResources();
    m_deviceResources->CreateWindowSizeDependentResources();
}

void RendererDX12::BeginFrame()
{
    m_deviceResources->Prepare();
}

void RendererDX12::EndFrame()
{
    m_deviceResources->Present();
}

void RendererDX12::OnResize(int width, int height)
{
    m_deviceResources->WindowSizeChanged(width, height);
}
