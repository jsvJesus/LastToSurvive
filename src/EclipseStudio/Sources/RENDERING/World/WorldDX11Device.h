#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>

struct WorldDX11DeviceCaps
{
    D3D_FEATURE_LEVEL FeatureLevel;
    bool bDebugDevice;
    bool bInitialized;
};

bool WorldDX11Device_Init();
void WorldDX11Device_Shutdown();

bool WorldDX11Device_IsInitialized();

ID3D11Device* WorldDX11Device_GetDevice();
ID3D11DeviceContext* WorldDX11Device_GetContext();

const WorldDX11DeviceCaps& WorldDX11Device_GetCaps();

#else

struct WorldDX11DeviceCaps
{
    int FeatureLevel;
    bool bDebugDevice;
    bool bInitialized;
};

static inline bool WorldDX11Device_Init()
{
    return false;
}

static inline void WorldDX11Device_Shutdown()
{
}

static inline bool WorldDX11Device_IsInitialized()
{
    return false;
}

static inline void* WorldDX11Device_GetDevice()
{
    return 0;
}

static inline void* WorldDX11Device_GetContext()
{
    return 0;
}

static inline const WorldDX11DeviceCaps& WorldDX11Device_GetCaps()
{
    static WorldDX11DeviceCaps Caps = {};
    return Caps;
}

#endif // LTS_STUDIO_DX11