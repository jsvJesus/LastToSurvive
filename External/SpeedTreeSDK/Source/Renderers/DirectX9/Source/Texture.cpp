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
#include "Utilities/Utility.h"
#include "Core/PerlinNoiseKernel.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CTextureDirectX9 static member variables

st_int32 CTextureDirectX9::m_nMaxAnisotropy = 0;


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::Load

st_bool CTextureDirectX9::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
    st_bool bSuccess = false;

    ST_UNREF_PARAM(nMaxAnisotropy);

    if (pFilename && strlen(pFilename) > 0)
    {
        // get file system pointer from Core lib
        CFileSystem* pFileSystem = CFileSystemInterface::Get( );
        assert(pFileSystem);

        // create temporary buffer
        CFixedString strTextureFilename = pFileSystem->CleanPlatformFilename(pFilename);
        const size_t c_siBufferSize = pFileSystem->FileSize(strTextureFilename.c_str( ));
        if (c_siBufferSize > 0)
        {
            st_byte* pTextureBuffer = pFileSystem->LoadFile(strTextureFilename.c_str( ));
            {
                assert(DX9::Device( ));
                bSuccess = SUCCEEDED(D3DXCreateTextureFromFileInMemoryEx(DX9::Device( ), 
                                                                         pTextureBuffer,
                                                                         UINT(c_siBufferSize),
                                                                         D3DX_DEFAULT, 
                                                                         D3DX_DEFAULT, 
                                                                         D3DX_DEFAULT, 
                                                                         0, 
                                                                         D3DFMT_UNKNOWN,
                                                                         D3DPOOL_MANAGED, 
                                                                         D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR,
                                                                         D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, 
                                                                         0, 
                                                                         NULL, 
                                                                         NULL, 
                                                                         &m_pTexture));

                // fill out texture info struct
                D3DSURFACE_DESC sDesc;
                if (SUCCEEDED(m_pTexture->GetLevelDesc(0, &sDesc)))
                {
                    m_sInfo.m_nWidth = st_int32(sDesc.Width);
                    m_sInfo.m_nHeight = st_int32(sDesc.Height);
                }

                pFileSystem->Release(pTextureBuffer);

                if (bSuccess)
                    m_nMaxAnisotropy = nMaxAnisotropy;
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  D3dColorFill
//
//  Utility function used by D3DXFillTexture in CTextureDirectX9::LoadColor below

VOID WINAPI D3dColorFill(D3DXVECTOR4* pOut, const D3DXVECTOR2* /*pTexCoord*/, const D3DXVECTOR2* /*pTexelSize*/, LPVOID pData)
{
    D3DCOLORVALUE* pColor = (D3DCOLORVALUE*) pData;
    *pOut = D3DXVECTOR4(pColor->r, pColor->g, pColor->b, pColor->a);
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::LoadColor

st_bool CTextureDirectX9::LoadColor(st_uint32 uiColor)
{
    st_bool bSuccess = false;

    const st_uint32 c_uiTextureSize = 4;

    D3DCOLORVALUE d3dColor;
    d3dColor.r = st_float32((uiColor & 0xff000000) >> 24) / 255.0f;
    d3dColor.g = st_float32((uiColor & 0x00ff0000) >> 16) / 255.0f;
    d3dColor.b = st_float32((uiColor & 0x0000ff00) >> 8) / 255.0f;
    d3dColor.a = st_float32((uiColor & 0x000000ff) >> 0) / 255.0f;

    assert(DX9::Device( ));
    bSuccess = SUCCEEDED(D3DXCreateTexture(DX9::Device( ), c_uiTextureSize, c_uiTextureSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture)) &&
               SUCCEEDED(D3DXFillTexture(m_pTexture, D3dColorFill, &d3dColor));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  D3dAlphaNoiseFill
//
//  Utility function used by D3DXFillTexture in CTextureDirectX9::LoadAlphaNoise below

struct SNoiseFillData
{
    CRandom     m_cRandom;
    st_float32  m_fLowNoise;
    st_float32  m_fHighNoise;
};

VOID WINAPI D3dAlphaNoiseFill(D3DXVECTOR4* pOut, const D3DXVECTOR2* /*pTexCoord*/, const D3DXVECTOR2* /*pTexelSize*/, LPVOID pData)
{
    SNoiseFillData* pFillData = reinterpret_cast<SNoiseFillData*>(pData);
    assert(pFillData);

    *pOut = D3DXVECTOR4(pFillData->m_cRandom.GetFloat(pFillData->m_fLowNoise, pFillData->m_fHighNoise), 
                        pFillData->m_cRandom.GetFloat(pFillData->m_fLowNoise, pFillData->m_fHighNoise), 
                        pFillData->m_cRandom.GetFloat(pFillData->m_fLowNoise, pFillData->m_fHighNoise), 
                        pFillData->m_cRandom.GetFloat(pFillData->m_fLowNoise, pFillData->m_fHighNoise));
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::LoadNoise

st_bool CTextureDirectX9::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
    st_bool bSuccess = false;

    if (nWidth > 4 && nHeight > 4 &&
        nWidth <= 4096 && nHeight <= 4096)
    {
        assert(DX9::Device( ));

        SNoiseFillData sFillData;
        sFillData.m_fLowNoise = fLowNoise;
        sFillData.m_fHighNoise = fHighNoise;

        bSuccess = D3DXCreateTexture(DX9::Device( ), UINT(nWidth), UINT(nHeight), 0, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_pTexture) == S_OK &&
                   D3DXFillTexture(m_pTexture, D3dAlphaNoiseFill, &sFillData) == S_OK;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  D3dPerlinNoiseFill

VOID WINAPI D3dPerlinNoiseFill(D3DXVECTOR4* pOut, const D3DXVECTOR2* pTexCoord, const D3DXVECTOR2* /*pTexelSize*/, LPVOID pData)
{
    assert(pTexCoord);

    CPerlinNoiseKernel* pKernel = reinterpret_cast<CPerlinNoiseKernel*>(pData);
    assert(pKernel);

    const st_float32 c_afSampleOffsets[4][2] = 
    {
        { -0.5f, -0.5f }, { -0.5f,  0.5f }, {  0.5f, -0.5f }, {  0.5f,  0.5f }
    };

    st_int32 nCol = st_int32(pTexCoord->x * pKernel->GetSize( ));
    st_int32 nRow = st_int32(pTexCoord->y * pKernel->GetSize( ));

    *pOut = D3DXVECTOR4(pKernel->BilinearSample(nCol + c_afSampleOffsets[0][0], nRow + c_afSampleOffsets[0][1]),
                        pKernel->BilinearSample(nCol + c_afSampleOffsets[1][0], nRow + c_afSampleOffsets[1][1]),
                        pKernel->BilinearSample(nCol + c_afSampleOffsets[2][0], nRow + c_afSampleOffsets[2][1]),
                        pKernel->BilinearSample(nCol + c_afSampleOffsets[3][0], nRow + c_afSampleOffsets[3][1]));
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::LoadPerlinNoiseKernel

st_bool CTextureDirectX9::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
    st_bool bSuccess = false;

    ST_UNREF_PARAM(nDepth);

    if (nWidth > 4 && nHeight > 4 &&
        nWidth <= 4096 && nHeight <= 4096)
    {
        assert(DX9::Device( ));

        CPerlinNoiseKernel cKernel(nWidth);

        bSuccess = D3DXCreateTexture(DX9::Device( ), UINT(nWidth), UINT(nHeight), 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture) == S_OK &&
                   D3DXFillTexture(m_pTexture, D3dPerlinNoiseFill, &cKernel) == S_OK;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::ReleaseGfxResources

st_bool CTextureDirectX9::ReleaseGfxResources(void)
{
    st_bool bSuccess = false;

    if (m_pTexture)
    {
        ST_SAFE_RELEASE(m_pTexture);
        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::SetSamplerStates

void CTextureDirectX9::SetSamplerStates(void)
{
    // set up standard samplers, possibly with anisotropic filtering
    for (st_int32 i = TEXTURE_REGISTER_DIFFUSE; i <= TEXTURE_REGISTER_SHADOW_MAP_0 - 1; ++i)
    {
        DX9::Device( )->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        DX9::Device( )->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        if (m_nMaxAnisotropy > 1)
        {
            DX9::Device( )->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
            DX9::Device( )->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, m_nMaxAnisotropy);
        }
        else
            DX9::Device( )->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }

    // set up shadow map samplers
    for (st_int32 i = TEXTURE_REGISTER_SHADOW_MAP_0; i <= TEXTURE_REGISTER_SHADOW_MAP_3; ++i)
    {
        DX9::Device( )->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        DX9::Device( )->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
        DX9::Device( )->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX9::CreateSharedSamplers

void CTextureDirectX9::CreateSharedSamplers(st_int32 nMaxAnisotropy)
{
    ST_UNREF_PARAM(nMaxAnisotropy);

    // intentionally empty
}

