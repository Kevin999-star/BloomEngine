#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include "DeviceResources.h"
#include <gameinput.h>
#include <dstorage.h>


class RendererDX12
{
public:
    RendererDX12();
    ~RendererDX12();

    void Initialize(void* windowHandle, int width, int height);
    void BeginFrame();
    void EndFrame();
    void OnResize(int width, int height);

private:
    std::unique_ptr<DX::DeviceResources> m_deviceResources;
};
