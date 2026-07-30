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
//  CStateBlockDirectX9::CStateBlockDirectX9

CStateBlockDirectX9::CStateBlockDirectX9( ) :
    m_bAlphaToCoverageSupported(false),
    m_bATI(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX9::Init

st_bool CStateBlockDirectX9::Init(const SAppState& sAppState, const SRenderState& sRenderState)
{
    st_bool bSuccess = true;

    m_sAppState = sAppState;
    m_sRenderState = sRenderState;

    // is alpha-to-coverage supported, and if so, is ATI mode needed?
    IDirect3D9* pD3D = NULL;
    if (SUCCEEDED(DX9::Device( )->GetDirect3D(&pD3D)))
    {
        D3DCAPS9 sCaps;
        pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &sCaps);
        if (sCaps.PixelShaderVersion >= D3DVS_VERSION(3, 0))
        {
            m_bAlphaToCoverageSupported = SUCCEEDED(pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C')));

            // no direct A2C check for ATI, so the check for ps_3_0 has to be enough
            if (!m_bAlphaToCoverageSupported)
            {
                m_bAlphaToCoverageSupported = true;
                m_bATI = true;
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  Utility Function: FloatToDWord

inline DWORD FloatToDWord(FLOAT f) 
{
    return *((DWORD*) &f); 
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX9::Bind

st_bool CStateBlockDirectX9::Bind(void) const
{
    st_bool bSuccess = true;

    // blending
    if (m_sRenderState.m_bBlending)
    {
        DX9::Device( )->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        DX9::Device( )->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        DX9::Device( )->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    else
        DX9::Device( )->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // color mask
    DWORD dwColorMask = D3DCOLORWRITEENABLE_ALPHA;
    if (m_sRenderState.m_eRenderPass == RENDER_PASS_MAIN)
        dwColorMask += D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE;
    DX9::Device( )->SetRenderState(D3DRS_COLORWRITEENABLE, dwColorMask);
    DX9::Device( )->SetRenderState(D3DRS_COLORWRITEENABLE1, dwColorMask);
    DX9::Device( )->SetRenderState(D3DRS_COLORWRITEENABLE2, dwColorMask);
    DX9::Device( )->SetRenderState(D3DRS_COLORWRITEENABLE3, dwColorMask);

    // depth mask & testing function
    if (m_sAppState.m_eOverrideDepthTest == SAppState::OVERRIDE_DEPTH_TEST_DISABLE)
        DX9::Device( )->SetRenderState(D3DRS_ZENABLE, FALSE);
    else
        DX9::Device( )->SetRenderState(D3DRS_ZENABLE, TRUE);
    if (!m_sAppState.m_bDepthPrepass)
    {
        DX9::Device( )->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        DX9::Device( )->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
    }
    else
    {
        if (m_sRenderState.m_eRenderPass == RENDER_PASS_DEPTH_PREPASS)
        {
            DX9::Device( )->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            DX9::Device( )->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
        }
        else
        {
            DX9::Device( )->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            DX9::Device( )->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
        }
    }

    // face culling
    if (m_sRenderState.m_eFaceCulling == CULLTYPE_BACK)
        DX9::Device( )->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    else if (m_sRenderState.m_eFaceCulling == CULLTYPE_FRONT)
        DX9::Device( )->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    else
        DX9::Device( )->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // polygon offset
    if (m_sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST)
    {
        // carefully tuned values for use with SpeedTree's reference application; change as needed
        const st_float32 c_fFactor = 1.0f;
        const st_float32 c_fUnits = 0.0004f;

        DX9::Device( )->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, FloatToDWord(c_fFactor));
        DX9::Device( )->SetRenderState(D3DRS_DEPTHBIAS, FloatToDWord(c_fUnits));
    }
    else
    {
        DX9::Device( )->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, FloatToDWord(0.0f));
        DX9::Device( )->SetRenderState(D3DRS_DEPTHBIAS, FloatToDWord(0.0f));
    }

    // multisampling
    DX9::Device( )->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, m_sAppState.m_bMultisampling ? TRUE : FALSE);

    // alpha-to-coverage
    const st_bool c_bAlphaToCoverageEnabled = (m_sAppState.m_bAlphaToCoverage && m_bAlphaToCoverageSupported);
    if (m_bATI)
        DX9::Device( )->SetRenderState(D3DRS_POINTSIZE, c_bAlphaToCoverageEnabled ? (D3DFORMAT)MAKEFOURCC('A', '2', 'M', '1') : D3DFMT_UNKNOWN);
    else
    {
        if (c_bAlphaToCoverageEnabled)
        {
            DX9::Device( )->SetRenderState(D3DRS_ALPHAREF, DWORD(0.0f));
            DX9::Device( )->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            DX9::Device( )->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        }
        else
            DX9::Device( )->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

        DX9::Device( )->SetRenderState(D3DRS_ADAPTIVETESS_Y, c_bAlphaToCoverageEnabled ? (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C') : D3DFMT_UNKNOWN);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX9::ReleaseGfxResources

void CStateBlockDirectX9::ReleaseGfxResources(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX9::GetAppState

const SAppState& CStateBlockDirectX9::GetAppState(void) const
{
    return m_sAppState;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX9::GetRenderState

const SRenderState& CStateBlockDirectX9::GetRenderState(void) const
{
    return m_sRenderState;
}





