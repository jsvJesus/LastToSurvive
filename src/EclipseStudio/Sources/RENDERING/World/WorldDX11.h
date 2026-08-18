#pragma once

#include "r3dRendererConfig.h"

struct WorldDX11FrameDesc
{
    int Width;
    int Height;
    float NearClip;
    float FarClip;
    bool DirectPresent;
};

#if LTS_STUDIO_DX11

bool WorldDX11_Init();
void WorldDX11_Shutdown();

bool WorldDX11_IsAvailable();
bool WorldDX11_Render(const WorldDX11FrameDesc& Desc);
bool WorldDX11_Present();
void WorldDX11_DrawDebugPreviewDX9();

#else

static inline bool WorldDX11_Init()
{
    return false;
}

static inline void WorldDX11_Shutdown()
{
}

static inline bool WorldDX11_IsAvailable()
{
    return false;
}

static inline bool WorldDX11_Render(const WorldDX11FrameDesc&)
{
    return false;
}

static inline bool WorldDX11_Present()
{
    return false;
}

static inline void WorldDX11_DrawDebugPreviewDX9()
{
}

#endif // LTS_STUDIO_DX11
