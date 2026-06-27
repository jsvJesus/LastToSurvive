#pragma once

#include "r3dRendererConfig.h"
#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

bool RenderDX11_Init();
void RenderDX11_Shutdown();

bool RenderDX11_IsReady();

bool RenderDX11_RenderWorld(
    const WorldDX11FrameDesc& Desc
);

#endif // LTS_STUDIO_DX11