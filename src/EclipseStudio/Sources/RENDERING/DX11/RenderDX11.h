#pragma once

#include "r3dRendererConfig.h"
#include "rendering/World/WorldDX11.h"

class r3dTexture;

#if LTS_STUDIO_DX11

struct RenderDX11SunGlareSettings
{
    float Threshold[4];
    float Tint[10][4];
    float TexTransform[10][4];

    // x = NumSunglares
    // y/z/w = reserved
    float Params[4];
};

bool RenderDX11_ApplySunGlare(
    const RenderDX11SunGlareSettings& Settings,
    r3dTexture* ShadeTexture
);

bool RenderDX11_Init();
void RenderDX11_Shutdown();

bool RenderDX11_IsReady();

bool RenderDX11_RenderWorld(
    const WorldDX11FrameDesc& Desc
);

void RenderDX11_DrawDebugPreviewDX9();

#endif // LTS_STUDIO_DX11
