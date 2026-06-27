#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

static void DrawWorldDX11_LogOnce()
{
    static bool bLogged = false;

    if (bLogged)
        return;

    bLogged = true;

    OutputDebugStringA(
        "[RenderDX11] DrawWorldDX11.hpp included. "
        "DX11 world draw path is prepared.\n"
    );
}

static bool DrawWorldDX11_BeginFrame(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    DrawWorldDX11_LogOnce();

    return true;
}

static bool DrawWorldDX11_DepthPrepass(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    // TODO:
    // DX9 analogue in RenderDeferredScene.hpp:
    // - r_z_prepass_method
    // - GameWorld().Draw(rsDepthPrepass)
    // - Terrain depth path
    //
    // DX11 version must later:
    // - set depth-only shaders
    // - bind DSV
    // - draw terrain depth
    // - draw static/skinned depth

    return true;
}

static bool DrawWorldDX11_FillGBuffer(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    // TODO:
    // DX9 analogue:
    // - Terrain1/Terrain2 deferred pass
    // - GameWorld().Draw(rsFillGBuffer)
    // - GameWorld().Draw(rsFillGBufferEffects)
    // - GameWorld().Draw(rsFillGBufferAfterEffects)
    //
    // DX11 version must later:
    // - create input layouts
    // - convert shader constants
    // - bind gbuffer RTVs
    // - draw terrain
    // - draw objects
    // - draw characters

    return true;
}

static bool DrawWorldDX11_Lighting(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    // TODO:
    // DX9 analogue:
    // - deferred lighting
    // - sun light
    // - point/spot lights
    // - SSAO/post later
    //
    // For now this is empty.

    return true;
}

static bool DrawWorldDX11_EndFrame(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    return true;
}

#endif // LTS_STUDIO_DX11