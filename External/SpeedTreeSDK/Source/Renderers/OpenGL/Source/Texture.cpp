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

#include "TgaFormat.h"
#include "NvDDS.h"
#include "Core/PerlinNoiseKernel.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::Load

st_bool CTextureOpenGL::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
    CFixedString strExtension = CFixedString(pFilename).Extension( );

#ifdef __GNUC__
    if (strcasecmp(strExtension.c_str( ), "tga") == 0)
#else
    if (_stricmp(strExtension.c_str( ), "tga") == 0)
#endif
        return LoadTGA(pFilename, nMaxAnisotropy);
    else
        return LoadDDS(pFilename, nMaxAnisotropy);
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOpenGL::LoadColor

st_bool CTextureOpenGL::LoadColor(st_uint32 uiColor)
{
    glGenTextures(1, &m_uiTexture);
    //glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_uiTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const st_uint32 c_uiTextureSize = 4;
    st_uint32 aTexture[c_uiTextureSize * c_uiTextureSize];

    const st_uint32 c_uiFinalColor = ((uiColor & 0x000000ff) << 24) + ((uiColor & 0x0000ff00) << 8) + ((uiColor & 0x00ff0000) >> 8) + ((uiColor & 0xff000000) >> 24);
    for (st_uint32 i = 0; i < c_uiTextureSize * c_uiTextureSize; ++i)
        aTexture[i] = c_uiFinalColor;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c_uiTextureSize, c_uiTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, aTexture);

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOpenGL::LoadNoise

st_bool CTextureOpenGL::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
    st_bool bSuccess = false;

    if (nWidth > 4 && nHeight > 4 &&
        nWidth <= 4096 && nHeight <= 4096)
    {
        glGenTextures(1, &m_uiTexture);
        glBindTexture(GL_TEXTURE_2D, m_uiTexture);

        // texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // allocate a buffer and fill it with random noise pattern
        st_int32 nNumTexels = nWidth * nHeight;
        CArray<st_byte> vTexture(nNumTexels);
        CRandom cRandom;
        for (st_int32 i = 0; i < nNumTexels; ++i)
            vTexture[i] = st_byte(cRandom.GetInteger(st_int32(fLowNoise * 255), st_int32(fHighNoise * 255)));

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, nWidth, nHeight, 0, GL_RED, GL_UNSIGNED_BYTE, &vTexture[0]);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOpenGL::LoadPerlinNoiseKernel

st_bool CTextureOpenGL::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
    st_bool bSuccess = false;

    ST_UNREF_PARAM(nDepth);

    if (nWidth > 4 && nHeight > 4 &&
        nWidth <= 4096 && nHeight <= 4096)
    {
        glGenTextures(1, &m_uiTexture);
        glBindTexture(GL_TEXTURE_2D, m_uiTexture);

        // texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        CPerlinNoiseKernel cKernel(nWidth);

        const st_float32 c_afSampleOffsets[4][2] = 
        {
            { -0.5f, -0.5f }, { -0.5f,  0.5f }, {  0.5f, -0.5f }, {  0.5f,  0.5f }
        };

        CArray<st_byte> vBuffer(nWidth * nHeight * 4);
        for (st_int32 nRow = 0; nRow < nHeight; ++nRow)
        {
            st_byte* pTexels = &vBuffer[0] + nRow * (nWidth * 4);
            for (st_int32 nCol = 0; nCol < nWidth; ++nCol)
            {
                *pTexels++ = st_byte(255 * cKernel.BilinearSample(nCol + c_afSampleOffsets[0][0], nRow + c_afSampleOffsets[0][1])); // red
                *pTexels++ = st_byte(255 * cKernel.BilinearSample(nCol + c_afSampleOffsets[1][0], nRow + c_afSampleOffsets[1][1])); // green
                *pTexels++ = st_byte(255 * cKernel.BilinearSample(nCol + c_afSampleOffsets[2][0], nRow + c_afSampleOffsets[2][1])); // blue
                *pTexels++ = st_byte(255 * cKernel.BilinearSample(nCol + c_afSampleOffsets[3][0], nRow + c_afSampleOffsets[3][1])); // alpha
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &vBuffer[0]);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::LoadTGA

st_bool CTextureOpenGL::LoadTGA(const CFixedString& strFilename, st_int32 nMaxAnisotropy)
{
    CTgaFormat cTarga;
    if (cTarga.Read(strFilename.c_str( ), true))
    {
        m_uiTexture = cTarga.CreateOpenGLTexture(nMaxAnisotropy);

        m_sInfo.m_nWidth = cTarga.GetWidth( );
        m_sInfo.m_nHeight = cTarga.GetHeight( );
    }

    return m_uiTexture != 0u;
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::LoadDDS

st_bool CTextureOpenGL::LoadDDS(const CFixedString& strFilename, st_int32 nMaxAnisotropy)
{
    // get file system pointer from Core lib
    CFileSystem* pFileSystem = CFileSystemInterface::Get( );
    assert(pFileSystem);

    // create temporary buffer
    const size_t c_siBufferSize = pFileSystem->FileSize(strFilename.c_str( ));
    if (c_siBufferSize > 0)
    {
        st_byte* pTextureBuffer = pFileSystem->LoadFile(strFilename.c_str( ));
        if (pTextureBuffer)
        {
            CNvDDS::SDDSHeader sFileHeader;
            m_uiTexture = CNvDDS::LoadFromData(strFilename.c_str( ), m_sInfo, pTextureBuffer, st_int32(c_siBufferSize), GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, sFileHeader, false, nMaxAnisotropy);
            pFileSystem->Release(pTextureBuffer);

            m_sInfo.m_nWidth = sFileHeader.m_uiWidth;
            m_sInfo.m_nHeight = sFileHeader.m_uiHeight;
        }
    }

    return m_uiTexture != 0u;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOpenGL::ReleaseGfxResources

st_bool CTextureOpenGL::ReleaseGfxResources(void)
{
    st_bool bSuccess = false;

    if (m_uiTexture > 0)
    {
        glDeleteTextures(1, &m_uiTexture);
        m_uiTexture = 0;

        bSuccess = true;
    }

    return bSuccess;
}


