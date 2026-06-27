#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>

struct WorldDX11ResourcesDesc
{
    int Width;
    int Height;
    DXGI_FORMAT ColorFormat;
    DXGI_FORMAT DepthFormat;
};

struct WorldDX11ResourcesState
{
    int Width;
    int Height;
    DXGI_FORMAT ColorFormat;
    DXGI_FORMAT DepthFormat;
    bool bInitialized;
};

bool WorldDX11Resources_Init(
    const WorldDX11ResourcesDesc& Desc
);

void WorldDX11Resources_Shutdown();

bool WorldDX11Resources_Resize(
    const WorldDX11ResourcesDesc& Desc
);

bool WorldDX11Resources_IsInitialized();

ID3D11Texture2D* WorldDX11Resources_GetColorTexture();
ID3D11Texture2D* WorldDX11Resources_GetDepthTexture();

ID3D11RenderTargetView* WorldDX11Resources_GetColorRTV();
ID3D11DepthStencilView* WorldDX11Resources_GetDepthDSV();

const D3D11_VIEWPORT& WorldDX11Resources_GetViewport();
const WorldDX11ResourcesState& WorldDX11Resources_GetState();

void WorldDX11Resources_Bind();
void WorldDX11Resources_Clear(
    float R,
    float G,
    float B,
    float A
);

#else

struct WorldDX11ResourcesDesc
{
    int Width;
    int Height;
    int ColorFormat;
    int DepthFormat;
};

struct WorldDX11ResourcesState
{
    int Width;
    int Height;
    int ColorFormat;
    int DepthFormat;
    bool bInitialized;
};

static inline bool WorldDX11Resources_Init(
    const WorldDX11ResourcesDesc&
)
{
    return false;
}

static inline void WorldDX11Resources_Shutdown()
{
}

static inline bool WorldDX11Resources_Resize(
    const WorldDX11ResourcesDesc&
)
{
    return false;
}

static inline bool WorldDX11Resources_IsInitialized()
{
    return false;
}

static inline void* WorldDX11Resources_GetColorTexture()
{
    return 0;
}

static inline void* WorldDX11Resources_GetDepthTexture()
{
    return 0;
}

static inline void* WorldDX11Resources_GetColorRTV()
{
    return 0;
}

static inline void* WorldDX11Resources_GetDepthDSV()
{
    return 0;
}

#endif // LTS_STUDIO_DX11