///////////////////////////////////////////////////////////////////////
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#include "Renderers/DirectX9/DirectX9Renderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::CRenderTargetDirectX9

CRenderTargetDirectX9::CRenderTargetDirectX9( ) :
    m_pColorTexture(NULL),
    m_pColorSurface(NULL),
    m_pDepthStencilTexture(NULL),
    m_pDepthStencilSurface(NULL),
    m_nWidth(-1),
    m_nHeight(-1),
    m_eType(RENDER_TARGET_TYPE_COLOR)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::~CRenderTargetDirectX9

CRenderTargetDirectX9::~CRenderTargetDirectX9( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::InitGfx

st_bool CRenderTargetDirectX9::InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
    st_bool bSuccess = false;

    ST_UNREF_PARAM(nNumSamples); // multisampling not supported in DX9 render-to-texture

    if (nWidth > 0 && nHeight > 0)
    {
        m_eType = eType;
        m_nWidth = nWidth;
        m_nHeight = nHeight;

        if (eType == RENDER_TARGET_TYPE_COLOR)
        {
            bSuccess = (SUCCEEDED(DX9::Device( )->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pColorTexture, NULL)) &&
                        SUCCEEDED(m_pColorTexture->GetSurfaceLevel(0, &m_pColorSurface)));
        }
        else if (eType == RENDER_TARGET_TYPE_DEPTH)
        {
            bSuccess = (SUCCEEDED(DX9::Device( )->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_DEPTHSTENCIL, D3DFORMAT(MAKEFOURCC('I','N','T','Z')), D3DPOOL_DEFAULT, &m_pDepthStencilTexture, NULL)) &&
                        SUCCEEDED(m_pDepthStencilTexture->GetSurfaceLevel(0, &m_pDepthStencilSurface)));
        }
        else // assume RENDER_TARGET_TYPE_SHADOW_MAP
        {
            if (SUCCEEDED(DX9::Device( )->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &m_pColorTexture, NULL)) &&
                SUCCEEDED(DX9::Device( )->CreateTexture(nWidth, nHeight, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D16, D3DPOOL_DEFAULT, &m_pDepthStencilTexture, NULL)))
            {
                bSuccess = (SUCCEEDED(m_pColorTexture->GetSurfaceLevel(0, &m_pColorSurface)) &&
                            SUCCEEDED(m_pDepthStencilTexture->GetSurfaceLevel(0, &m_pDepthStencilSurface)));
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::ReleaseGfxResources

void CRenderTargetDirectX9::ReleaseGfxResources(void)
{
    ST_SAFE_RELEASE(m_pColorTexture);
    ST_SAFE_RELEASE(m_pColorSurface);
    ST_SAFE_RELEASE(m_pDepthStencilTexture);
    ST_SAFE_RELEASE(m_pDepthStencilSurface);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::Clear

void CRenderTargetDirectX9::Clear(const Vec4& vColor)
{
    DWORD dwDirectXBitField = (m_eType == RENDER_TARGET_TYPE_COLOR) ? D3DCLEAR_TARGET : D3DCLEAR_ZBUFFER;

    DX9::Device( )->Clear(0, NULL, dwDirectXBitField, D3DCOLOR_COLORVALUE(vColor.x, vColor.y, vColor.z, vColor.w), 1.0f, 0);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::SetAsTarget

st_bool CRenderTargetDirectX9::SetAsTarget(void)
{
    const CRenderTargetDirectX9* pTarget = this;

    return SetGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::ReleaseAsTarget

void CRenderTargetDirectX9::ReleaseAsTarget(void)
{
    const CRenderTargetDirectX9* pTarget = this;

    ReleaseGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::BindAsTexture

st_bool CRenderTargetDirectX9::BindAsTexture(st_int32 nRegisterIndex, st_bool bPointFilter) const
{
    if (m_eType == RENDER_TARGET_TYPE_COLOR)
        DX9::Device( )->SetTexture(nRegisterIndex, m_pColorTexture);
    else
        DX9::Device( )->SetTexture(nRegisterIndex, m_pDepthStencilTexture);

    if (m_eType == RENDER_TARGET_TYPE_SHADOW_MAP)
    {
        // use these filters to enable shadow map PCF
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }
    else
    {
        // set to point sampling to avoid unwanted filtering for one-to-one render target lookups
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MINFILTER, bPointFilter ? D3DTEXF_POINT : D3DTEXF_LINEAR);
        DX9::Device( )->SetSamplerState(nRegisterIndex, D3DSAMP_MAGFILTER, bPointFilter ? D3DTEXF_POINT : D3DTEXF_LINEAR);

    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::UnBindAsTexture

void CRenderTargetDirectX9::UnBindAsTexture(st_int32 nRegister) const
{
    ST_UNREF_PARAM(nRegister);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::OnResetDevice

void CRenderTargetDirectX9::OnResetDevice(void)
{
    InitGfx(m_eType, m_nWidth, m_nHeight, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::OnLostDevice

void CRenderTargetDirectX9::OnLostDevice(void)
{
    ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::SetGroupAsTarget

st_bool CRenderTargetDirectX9::SetGroupAsTarget(const CRenderTargetDirectX9** pTargets, st_int32 nNumTargets)
{
    st_bool bSuccess = false;

    assert(pTargets);
    assert(nNumTargets <= c_nMaxNumRenderTargets);
    assert(DX9::Device( ));

    if (nNumTargets > 0)
    {
        // preserve current viewport
        DX9::Device( )->GetViewport(&m_sPreviouslyActiveViewport);

        // preserve the currently-active render target (can only grab the first render target)
        DX9::Device( )->GetRenderTarget(0, &m_pPreviouslyActiveRenderTargetSurface);

        if (pTargets[0]->m_eType == RENDER_TARGET_TYPE_SHADOW_MAP)
        {
            DX9::Device( )->SetRenderTarget(0, pTargets[0]->m_pColorSurface);

            // depth-stencil
            DX9::Device( )->GetDepthStencilSurface(&m_pPreviouslyActiveDepthStencilSurface);
            DX9::Device( )->SetDepthStencilSurface(pTargets[0]->m_pDepthStencilSurface);
        }
        else
        {
            st_int32 nColorTargetIndex = 0;
            const CRenderTargetDirectX9* pDepthRenderTarget = NULL;
            for (st_int32 i = 0; i < nNumTargets; ++i)
            {
                assert(pTargets[i]);
                if (pTargets[i]->m_eType == RENDER_TARGET_TYPE_COLOR)
                    DX9::Device( )->SetRenderTarget(nColorTargetIndex++, pTargets[i]->m_pColorSurface);
                else
                    pDepthRenderTarget = pTargets[i];
            }

            // depth-stencil
            DX9::Device( )->GetDepthStencilSurface(&m_pPreviouslyActiveDepthStencilSurface);
            DX9::Device( )->SetDepthStencilSurface(pDepthRenderTarget ? pDepthRenderTarget->m_pDepthStencilSurface : NULL);
        }

        // set render target viewport
        D3DVIEWPORT9 sViewData = { 0, 0, pTargets[0]->m_nWidth, pTargets[0]->m_nHeight, 0.0f, 1.0f };
        DX9::Device( )->SetViewport(&sViewData);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX9::ReleaseGroupAsTarget

void CRenderTargetDirectX9::ReleaseGroupAsTarget(const CRenderTargetDirectX9** pTargets, st_int32 nNumTargets)
{
    ST_UNREF_PARAM(pTargets);
    ST_UNREF_PARAM(nNumTargets);

    DX9::Device( )->SetRenderTarget(0, m_pPreviouslyActiveRenderTargetSurface);
    for (st_uint32 i = 1; i < c_nMaxNumRenderTargets; ++i)
        DX9::Device( )->SetRenderTarget(i, NULL);

    DX9::Device( )->SetDepthStencilSurface(m_pPreviouslyActiveDepthStencilSurface);
    DX9::Device( )->SetViewport(&m_sPreviouslyActiveViewport);

    ST_SAFE_RELEASE(m_pPreviouslyActiveRenderTargetSurface);
    ST_SAFE_RELEASE(m_pPreviouslyActiveDepthStencilSurface);
}
