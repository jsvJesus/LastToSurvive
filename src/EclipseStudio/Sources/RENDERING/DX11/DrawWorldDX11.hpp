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

static void DrawWorldDX11_LogTerrainSkeletonOnce(
    const char* PassName,
    bool bGBuffer
)
{
    static bool bTerrainDepthLogged = false;
    static bool bTerrainGBufferLogged = false;

    bool* bLogged =
        bGBuffer
        ? &bTerrainGBufferLogged
        : &bTerrainDepthLogged;

    if (*bLogged)
        return;

    *bLogged = true;

    char Text[256] = {};
    sprintf_s(
        Text,
        "[RenderDX11] Terrain %s skeleton prepared. Terrain2=%p Terrain=%p\n",
        PassName ? PassName : "unknown",
        static_cast<void*>(Terrain2),
        static_cast<void*>(Terrain)
    );

    OutputDebugStringA(Text);
}

static bool DrawWorldDX11_TerrainDepth(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (!gDX11Context)
        return false;

    DrawWorldDX11_LogTerrainSkeletonOnce(
        "Depth",
        false
    );

    if (!RenderDX11_DrawTerrainDepth())
    {
        static bool bTerrainDepthSkippedLogged = false;

        if (!bTerrainDepthSkippedLogged)
        {
            bTerrainDepthSkippedLogged = true;
            OutputDebugStringA(
                "[RenderDX11] Terrain depth draw skipped\n"
            );
        }

        return true;
    }

    static bool bTerrainDepthDrawLogged = false;

    if (!bTerrainDepthDrawLogged)
    {
        bTerrainDepthDrawLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Terrain depth draw issued\n"
        );
    }

    return true;
}

static bool DrawWorldDX11_TerrainGBuffer(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (!gDX11Context)
        return false;

    DrawWorldDX11_LogTerrainSkeletonOnce(
        "GBuffer",
        true
    );

    if (!RenderDX11_DrawTerrainGBuffer())
    {
        static bool bTerrainGBufferSkippedLogged = false;

        if (!bTerrainGBufferSkippedLogged)
        {
            bTerrainGBufferSkippedLogged = true;
            OutputDebugStringA(
                "[RenderDX11] Terrain GBuffer draw skipped\n"
            );
        }

        return true;
    }

    static bool bTerrainGBufferDrawLogged = false;

    if (!bTerrainGBufferDrawLogged)
    {
        bTerrainGBufferDrawLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Terrain GBuffer draw issued\n"
        );
    }

    return true;
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

    if (!gDX11Context || !gDX11DepthDSV)
        return false;

    gDX11Context->OMSetRenderTargets(
        0,
        0,
        gDX11DepthDSV
    );

    gDX11Context->RSSetViewports(
        1,
        &gDX11Viewport
    );

    gDX11Context->OMSetDepthStencilState(
        gDX11DepthWriteLessEqual,
        0
    );

    const float BlendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    gDX11Context->OMSetBlendState(
        gDX11BlendOpaque,
        BlendFactor,
        0xffffffff
    );

    gDX11Context->RSSetState(
        gDX11RasterSolidBackCull
    );

    gDX11Context->ClearDepthStencilView(
        gDX11DepthDSV,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );

    static bool bDepthPrepassLogged = false;

    if (!bDepthPrepassLogged)
    {
        bDepthPrepassLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Depth prepass skeleton prepared\n"
        );
    }

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

    if (!DrawWorldDX11_TerrainDepth(Desc))
        return false;

    return true;
}

static bool DrawWorldDX11_FillGBuffer(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (
        !gDX11Context ||
        !gDX11GBufferColorRTV ||
        !gDX11GBufferNormalRTV ||
        !gDX11GBufferDepthLinearRTV ||
        !gDX11GBufferAuxRTV ||
        !gDX11DepthDSV
    )
    {
        return false;
    }

    ID3D11RenderTargetView* RTViews[4] =
    {
        gDX11GBufferColorRTV,
        gDX11GBufferNormalRTV,
        gDX11GBufferDepthLinearRTV,
        gDX11GBufferAuxRTV
    };

    gDX11Context->OMSetRenderTargets(
        4,
        RTViews,
        gDX11DepthDSV
    );

    gDX11Context->RSSetViewports(
        1,
        &gDX11Viewport
    );

    gDX11Context->OMSetDepthStencilState(
        gDX11DepthReadLessEqual,
        0
    );

    const float BlendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    gDX11Context->OMSetBlendState(
        gDX11BlendOpaque,
        BlendFactor,
        0xffffffff
    );

    gDX11Context->RSSetState(
        gDX11RasterSolidBackCull
    );

    static bool bFillGBufferLogged = false;

    if (!bFillGBufferLogged)
    {
        bFillGBufferLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Fill GBuffer skeleton prepared\n"
        );
    }

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

    if (!DrawWorldDX11_TerrainGBuffer(Desc))
        return false;

    return true;
}

static bool DrawWorldDX11_Lighting(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (!gDX11Context)
        return false;

    gDX11Context->OMSetDepthStencilState(
        gDX11DepthDisabled,
        0
    );

    const float BlendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    gDX11Context->OMSetBlendState(
        gDX11BlendOpaque,
        BlendFactor,
        0xffffffff
    );

    gDX11Context->RSSetState(
        gDX11RasterSolidNoCull
    );

    static bool bLightingLogged = false;

    if (!bLightingLogged)
    {
        bLightingLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Lighting skeleton prepared\n"
        );
    }

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

static bool DrawWorldDX11_Transparent(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (!gDX11Context)
        return false;

    gDX11Context->OMSetDepthStencilState(
        gDX11DepthReadLessEqual,
        0
    );

    const float BlendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    gDX11Context->OMSetBlendState(
        gDX11BlendAlpha,
        BlendFactor,
        0xffffffff
    );

    gDX11Context->RSSetState(
        gDX11RasterSolidNoCull
    );

    static bool bTransparentLogged = false;

    if (!bTransparentLogged)
    {
        bTransparentLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Transparent skeleton prepared\n"
        );
    }

    // TODO:
    // DX11 transparent pass later:
    // - alpha-tested grass/vegetation
    // - transparent objects
    // - water
    // - soft particles
    // - distortion
    //
    // For now this stage is intentionally empty.

    return true;
}

static bool DrawWorldDX11_Post(
    const WorldDX11FrameDesc& Desc
)
{
    (void)Desc;

    if (!gDX11Context)
        return false;

    gDX11Context->OMSetDepthStencilState(
        gDX11DepthDisabled,
        0
    );

    const float BlendFactor[4] =
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    gDX11Context->OMSetBlendState(
        gDX11BlendOpaque,
        BlendFactor,
        0xffffffff
    );

    gDX11Context->RSSetState(
        gDX11RasterSolidNoCull
    );

    static bool bPostLogged = false;

    if (!bPostLogged)
    {
        bPostLogged = true;
        OutputDebugStringA(
            "[RenderDX11] Post skeleton prepared\n"
        );
    }

    // TODO:
    // DX11 post stage later:
    // - combine lighting result
    // - fog
    // - tonemap or copy to composition target
    // - debug overlay/gbuffer compare
    //
    // For now this stage is intentionally empty.

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
