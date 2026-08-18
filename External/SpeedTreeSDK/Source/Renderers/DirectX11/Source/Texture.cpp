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

#include "Renderers/DirectX11/DirectX11Renderer.h"
#include "DDSTextureLoader.h"
#include "Core/PerlinNoiseKernel.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CTextureDirectX11 static member variables

ID3D11SamplerState* CTextureDirectX11::m_pSamplerLinearWrap = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pSamplerLinearClamped = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pSamplerPointClampedNoMipmap = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pSamplerShadowCompare = NULL;
st_int32 CTextureDirectX11::m_nSharedSamplerRefCount = 0;
st_int32 CTextureDirectX11::m_nMaxAnisotropy = 1;


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::Load

st_bool CTextureDirectX11::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
    if (pFilename && strlen(pFilename) > 0)
    {
        assert(DX11::Device( ));

        // get file system pointer from Core lib
        CFileSystem* pFileSystem = CFileSystemInterface::Get( );
        assert(pFileSystem);

        // set up a temporary buffer for reading the texture
        size_t siTextureFileSize = pFileSystem->FileSize(pFilename);
        if (siTextureFileSize > 0)
        {
            st_byte* pTextureBuffer = pFileSystem->LoadFile(pFilename);
            if (pTextureBuffer)
            {
                if (SUCCEEDED(DirectX::CreateDDSTextureFromMemoryEx(DX11::Device(),
                                                                    nullptr, 
                                                                    pTextureBuffer,
                                                                    siTextureFileSize,
                                                                    0,
                                                                    D3D11_USAGE_DEFAULT, 
                                                                    D3D11_BIND_SHADER_RESOURCE, 
                                                                    0, 
                                                                    0, 
                                                                    false, // force sRGB
                                                                    NULL,
                                                                    &m_pTexture,
                                                                    NULL)))
                {
                    assert(m_pTexture);
                    ST_NAME_DX11_OBJECT(m_pTexture, "speedtree texture shader resource view");

                    m_bIsGeneratedUniformColor = false;
                    m_nMaxAnisotropy = nMaxAnisotropy;

                    ++m_nSharedSamplerRefCount;

                    // extract texture dimensions
                    ID3D11Resource* pResource = NULL;
                    m_pTexture->GetResource(&pResource);
                    if (pResource)
                    {
                        D3D11_RESOURCE_DIMENSION eDimension;
                        pResource->GetType(&eDimension);
                        if (eDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                        {
                            ID3D11Texture2D* pTexture2d = reinterpret_cast<ID3D11Texture2D*>(pResource);

                            D3D11_TEXTURE2D_DESC sTexture2dDesc;
                            pTexture2d->GetDesc(&sTexture2dDesc);
                            m_sInfo.m_nWidth = sTexture2dDesc.Width;
                            m_sInfo.m_nHeight = sTexture2dDesc.Height;
                        }

                        pResource->Release( );
                    }
                }

                pFileSystem->Release(pTextureBuffer);
            }
        }
    }

    return (m_pTexture != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadColor

st_bool CTextureDirectX11::LoadColor(st_uint32 uiColor)
{
    st_bool bSuccess = false;

    CD3D11_TEXTURE2D_DESC sDesc;
    ZeroMemory(&sDesc, sizeof(sDesc));
    sDesc.Width = 4;
    sDesc.Height = 4;
    sDesc.MipLevels = sDesc.ArraySize = 1;
    sDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sDesc.SampleDesc.Count = 1;
    sDesc.Usage = D3D11_USAGE_DYNAMIC;
    sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Texture2D* pTexture = NULL;

    assert(DX11::Device( ));
    if (SUCCEEDED(DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture)))
    {
        ST_NAME_DX11_OBJECT(pTexture, "speedtree uniform color texture");

        D3D11_MAPPED_SUBRESOURCE sMappedTex;
        DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);

        for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
        {
            UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
            for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
            {
                *pTexels++ = UCHAR((uiColor & 0xff000000) >> 24); // red
                *pTexels++ = UCHAR((uiColor & 0x00ff0000) >> 16); // green
                *pTexels++ = UCHAR((uiColor & 0x0000ff00) >> 8);  // blue
                *pTexels++ = UCHAR((uiColor & 0x000000ff) >> 0);  // alpha
            }
        }

        DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

        bSuccess = SUCCEEDED(DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture));
        ST_SAFE_RELEASE(pTexture);

        if (bSuccess)
        {
            ST_NAME_DX11_OBJECT(m_pTexture, "speedtree uniform color shader resource view");
            ++m_nSharedSamplerRefCount;
        }
    }

    m_bIsGeneratedUniformColor = true;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadNoise

st_bool CTextureDirectX11::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
    st_bool bSuccess = false;

    if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
    {
        assert(DX11::Device( ));
        CRandom cRandom;

        D3D11_TEXTURE2D_DESC sDesc;
        ZeroMemory(&sDesc, sizeof(sDesc));
        sDesc.Width = nWidth;
        sDesc.Height = nHeight;
        sDesc.MipLevels = sDesc.ArraySize = 1;
        sDesc.Format = DXGI_FORMAT_R8_UNORM;
        sDesc.SampleDesc.Count = 1;
        sDesc.Usage = D3D11_USAGE_DYNAMIC;
        sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ID3D11Texture2D* pTexture = NULL;
        if (DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture) == S_OK)
        {
            ST_NAME_DX11_OBJECT(pTexture, "speedtree noise texture");

            D3D11_MAPPED_SUBRESOURCE sMappedTex;
            DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);

            for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
            {
                UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
                for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
                    *pTexels++ = UCHAR(cRandom.GetInteger(st_int32(fLowNoise * 255), st_int32(fHighNoise * 255)));
            }

            DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

            bSuccess = (DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture) == S_OK);
            pTexture->Release( );

            if (bSuccess)
            {
                ST_NAME_DX11_OBJECT(m_pTexture, "speedtree noise shader resource view");
                ++m_nSharedSamplerRefCount;
            }
        }
    }

    m_bIsGeneratedUniformColor = true;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadPerlinNoiseKernel

st_bool CTextureDirectX11::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
    st_bool bSuccess = false;

    ST_UNREF_PARAM(nDepth);
    if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
    {
        assert(DX11::Device( ));
        CRandom cRandom;

        D3D11_TEXTURE2D_DESC sDesc;
        ZeroMemory(&sDesc, sizeof(sDesc));
        sDesc.Width = nWidth;
        sDesc.Height = nHeight;
        sDesc.MipLevels = sDesc.ArraySize = 1;
        sDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sDesc.SampleDesc.Count = 1;
        sDesc.Usage = D3D11_USAGE_DYNAMIC;
        sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ID3D11Texture2D* pTexture = NULL;
        if (DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture) == S_OK)
        {
            ST_NAME_DX11_OBJECT(pTexture, "speedtree perlin noise kernel texture");

            D3D11_MAPPED_SUBRESOURCE sMappedTex;
            DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);
            {
                CPerlinNoiseKernel cKernel(nWidth);

                const st_float32 c_afSampleOffsets[4][2] = 
                {
                    { -0.5f, -0.5f }, { -0.5f,  0.5f }, {  0.5f, -0.5f }, {  0.5f,  0.5f }
                };

                for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
                {
                    UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
                    for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
                    {
                        *pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[0][0], uiRow + c_afSampleOffsets[0][1])); // red
                        *pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[1][0], uiRow + c_afSampleOffsets[1][1])); // green
                        *pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[2][0], uiRow + c_afSampleOffsets[2][1])); // blue
                        *pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[3][0], uiRow + c_afSampleOffsets[3][1])); // alpha
                    }
                }
            }
            DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

            bSuccess = (DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture) == S_OK);
            pTexture->Release( );

            if (bSuccess)
            {
                ST_NAME_DX11_OBJECT(m_pTexture, "speedtree perlin noise kernel shader resource view");
                ++m_nSharedSamplerRefCount;
            }
        }
    }

    m_bIsGeneratedUniformColor = true;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::ReleaseGfxResources

st_bool CTextureDirectX11::ReleaseGfxResources(void)
{
    st_bool bSuccess = true;

    if (m_pTexture)
    {
        if (--m_nSharedSamplerRefCount == 0)
        {
            ReleaseSampler(m_pSamplerLinearWrap);
            ReleaseSampler(m_pSamplerLinearClamped);
            ReleaseSampler(m_pSamplerPointClampedNoMipmap);
            ReleaseSampler(m_pSamplerShadowCompare);
        }

        ReleaseTexture(m_pTexture);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::SetSamplerStates

void CTextureDirectX11::SetSamplerStates(void)
{
    // create the shared samplers if they haven't already been
    if (!m_pSamplerLinearWrap)
        CreateSharedSamplers(m_nMaxAnisotropy);

    if (m_pSamplerLinearWrap && m_pSamplerLinearClamped && m_pSamplerPointClampedNoMipmap && m_pSamplerShadowCompare)
    {
        ID3D11SamplerState* apSamplers[SAMPLER_REGISTER_COUNT] =
        {
            m_pSamplerLinearWrap,
            m_pSamplerLinearClamped,
            m_pSamplerPointClampedNoMipmap,
            m_pSamplerShadowCompare
        };

        DX11::DeviceContext( )->PSSetSamplers(0, SAMPLER_REGISTER_COUNT, apSamplers);
        DX11::DeviceContext( )->VSSetSamplers(0, SAMPLER_REGISTER_COUNT, apSamplers);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::CreateSharedSamplers

void CTextureDirectX11::CreateSharedSamplers(st_int32 nMaxAnisotropy)
{
    // linear wrap
    D3D11_SAMPLER_DESC sLinearWrapSamplerDesc;
    ZeroMemory(&sLinearWrapSamplerDesc, sizeof(sLinearWrapSamplerDesc));
    sLinearWrapSamplerDesc.Filter = (nMaxAnisotropy > 1) ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sLinearWrapSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sLinearWrapSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sLinearWrapSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sLinearWrapSamplerDesc.MipLODBias = 0.0f;
    sLinearWrapSamplerDesc.MaxAnisotropy = nMaxAnisotropy;
    sLinearWrapSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    st_float32 afWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    memcpy(sLinearWrapSamplerDesc.BorderColor, afWhite, sizeof(afWhite));
    sLinearWrapSamplerDesc.MinLOD = 0.0f;
    sLinearWrapSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sLinearWrapSamplerDesc, &m_pSamplerLinearWrap)))
        ST_NAME_DX11_OBJECT(m_pSamplerLinearWrap, "speedtree standard sampler");

    // linear clamped
    D3D11_SAMPLER_DESC sLinearClampedSamplerDesc;
    ZeroMemory(&sLinearClampedSamplerDesc, sizeof(sLinearClampedSamplerDesc));
    sLinearClampedSamplerDesc.Filter = (nMaxAnisotropy > 1) ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sLinearClampedSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sLinearClampedSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sLinearClampedSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sLinearClampedSamplerDesc.MipLODBias = 0.0f;
    sLinearClampedSamplerDesc.MaxAnisotropy = nMaxAnisotropy;
    sLinearClampedSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sLinearClampedSamplerDesc.MinLOD = 0.0f;
    sLinearClampedSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sLinearClampedSamplerDesc, &m_pSamplerLinearClamped)))
        ST_NAME_DX11_OBJECT(m_pSamplerLinearClamped, "speedtree linear clamped sampler");

    // shadow map comparison sampler
    D3D11_SAMPLER_DESC sShadowCompareSamplerDesc;
    ZeroMemory(&sShadowCompareSamplerDesc, sizeof(sShadowCompareSamplerDesc));
    sShadowCompareSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sShadowCompareSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sShadowCompareSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sShadowCompareSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sShadowCompareSamplerDesc.MipLODBias = 0.0f;
    sShadowCompareSamplerDesc.MaxAnisotropy = 0;
    sShadowCompareSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    const st_float32 c_afWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    memcpy(sShadowCompareSamplerDesc.BorderColor, c_afWhite, sizeof(c_afWhite));
    sShadowCompareSamplerDesc.MinLOD = 0.0f;
    sShadowCompareSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sShadowCompareSamplerDesc, &m_pSamplerShadowCompare)))
        ST_NAME_DX11_OBJECT(m_pSamplerLinearClamped, "speedtree shadow map sampler");

    // point clamped no mipmap sampler
    D3D11_SAMPLER_DESC sPointSamplerDesc;
    ZeroMemory(&sPointSamplerDesc, sizeof(sPointSamplerDesc));
    sPointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sPointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // need wrap when accessing generated noise texture
    sPointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sPointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sPointSamplerDesc, &m_pSamplerPointClampedNoMipmap)))
        ST_NAME_DX11_OBJECT(m_pSamplerPointClampedNoMipmap, "speedtree point sampler");
}
