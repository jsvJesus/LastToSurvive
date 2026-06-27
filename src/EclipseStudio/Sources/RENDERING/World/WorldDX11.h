#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

struct StudioWorldDX11FrameDesc
{
    int Width;
    int Height;
    float NearClip;
    float FarClip;
};

bool StudioWorldDX11_IsAvailable();
bool StudioWorldDX11_RenderWorld(const StudioWorldDX11FrameDesc& Desc);

#else

struct StudioWorldDX11FrameDesc
{
    int Width;
    int Height;
    float NearClip;
    float FarClip;
};

static inline bool StudioWorldDX11_IsAvailable()
{
    return false;
}

static inline bool StudioWorldDX11_RenderWorld(const StudioWorldDX11FrameDesc&)
{
    return false;
}

#endif // LTS_STUDIO_DX11