#pragma once

#include "r3dRendererConfig.h"

struct WorldDX11FrameDesc
{
    int Width;
    int Height;
    float NearClip;
    float FarClip;
};

#if LTS_STUDIO_DX11

bool WorldDX11_IsAvailable();
bool WorldDX11_Render(const WorldDX11FrameDesc& Desc);

#else

static inline bool WorldDX11_IsAvailable()
{
    return false;
}

static inline bool WorldDX11_Render(const WorldDX11FrameDesc&)
{
    return false;
}

#endif // LTS_STUDIO_DX11