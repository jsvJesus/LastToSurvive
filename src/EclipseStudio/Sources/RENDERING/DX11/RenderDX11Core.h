#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>

struct RenderDX11CoreCreateDesc
{
    HWND Window;

    unsigned int Width;
    unsigned int Height;

    bool Windowed;
    bool EnableDebugLayer;

    RenderDX11CoreCreateDesc()
        : Window(0)
        , Width(1)
        , Height(1)
        , Windowed(true)
        , EnableDebugLayer(false)
    {
    }
};

class RenderDX11Core
{
public:
    RenderDX11Core();

    bool Initialize(
        const RenderDX11CoreCreateDesc& Desc
    );

    void Shutdown();

    bool IsReady() const;

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetContext() const;

    D3D_FEATURE_LEVEL GetFeatureLevel() const;

    bool Resize(
        unsigned int Width,
        unsigned int Height
    );

    bool CopyToBackBuffer(
        ID3D11Texture2D* SourceTexture
    );

    bool Present();

private:
    RenderDX11Core(
        const RenderDX11Core&
    );

    RenderDX11Core& operator=(
        const RenderDX11Core&
    );

private:
    ID3D11Device* Device_;
    ID3D11DeviceContext* Context_;

    D3D_FEATURE_LEVEL FeatureLevel_;

    bool EngineDeviceInitialized_;
};

RenderDX11Core& RenderDX11_GetCore();

#endif // LTS_STUDIO_DX11