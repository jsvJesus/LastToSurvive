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

#include "Core/Types.h"
#include "TgaFormat.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
// TGA header keys

const st_uchar c_acTGAheader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const st_uchar c_acTGAGrayScaleheader[12] = { 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const st_uchar c_acTGARLEheader[12] = { 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::CTgaFormat

CTgaFormat::CTgaFormat( ) :
    m_nHeight(0),
    m_nWidth(0),
    m_nDepth(0),
    m_pImageData(NULL),
    m_bDeleteData(true)
{
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::CTgaFormat

CTgaFormat::CTgaFormat(st_uchar* pImageData, st_int32 nWidth, st_int32 nHeight, st_int32 nDepth, st_bool bCopyData) :
    m_nHeight(nHeight),
    m_nWidth(nWidth),
    m_nDepth(nDepth),
    m_pImageData(NULL),
    m_bDeleteData(bCopyData)
{
    if (pImageData == NULL)
    {
        st_int32 nSize = m_nWidth * m_nHeight * m_nDepth;
        m_pImageData = st_new_array<st_uchar>(nSize, "CTGAFormat::st_uchar");
        memset(m_pImageData, 0, nSize);
    }
    else
    {
        if (bCopyData)
        {
            st_int32 nSize = m_nWidth * m_nHeight * m_nDepth;
            m_pImageData = st_new_array<st_uchar>(nSize, "CTGAFormat::st_uchar");
            memcpy(m_pImageData, pImageData, nSize);
        }
        else
        {
            m_pImageData = pImageData;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::~CTgaFormat

CTgaFormat::~CTgaFormat( )
{
    if (m_bDeleteData)
        st_delete_array<st_uchar>(m_pImageData);
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::Read

st_bool CTgaFormat::Read(const char* pFilename, st_bool bFlip)
{
    if (m_bDeleteData)
        st_delete_array<st_uchar>(m_pImageData);
    m_pImageData = NULL;

    st_bool bSuccess = false;

    FILE* pFile = fopen(pFilename, "rb");
    if (pFile != NULL)
    {
        st_uchar acTGAcompare[12];     // used to compare tga header
        st_uchar acDimensionBytes[6];  // first 6 useful bytes from the header

        fread((char*)acTGAcompare, sizeof(acTGAcompare), 1, pFile);
        memset(acTGAcompare + 7, 0, 5); // wipes out palette bits because we don't care as long as it doesn't have one

        if (memcmp(c_acTGAheader, acTGAcompare, sizeof(c_acTGAheader)) == 0)
        {
            // uncompressed TGA

            fread((char*)acDimensionBytes, sizeof(acDimensionBytes), 1, pFile);

            m_nWidth = (acDimensionBytes[1] << 8) + acDimensionBytes[0];
            m_nHeight = (acDimensionBytes[3] << 8) + acDimensionBytes[2];

            if (m_nWidth > 0 || m_nHeight > 0)
            {
                st_uint32 nBitsPerPixel = acDimensionBytes[4];

                if (nBitsPerPixel == 24 || nBitsPerPixel == 32)
                {
                    m_nDepth = nBitsPerPixel / 8;
                    st_int32 nImageSize = m_nWidth * m_nHeight * m_nDepth;

                    m_pImageData = st_new_array<st_uchar>(nImageSize, "CTgaFormat::st_uchar");
                    
                    fread((char*)m_pImageData, nImageSize, 1, pFile);

                    bSuccess = true;
                }
            }
        }
        else if (memcmp(c_acTGARLEheader, acTGAcompare, sizeof(c_acTGARLEheader)) == 0)
        {
            // rle compressed TGA

            fread((char*)acDimensionBytes, sizeof(acDimensionBytes), 1, pFile);

            m_nWidth = (acDimensionBytes[1] << 8) + acDimensionBytes[0];
            m_nHeight = (acDimensionBytes[3] << 8) + acDimensionBytes[2];

            if (m_nWidth > 0 || m_nHeight > 0)
            {
                st_uint32 nBitsPerPixel = acDimensionBytes[4];

                if (nBitsPerPixel == 24 || nBitsPerPixel == 32)
                {
                    m_nDepth = nBitsPerPixel / 8;
                    st_int32 nImageSize = m_nWidth * m_nHeight * m_nDepth;

                    m_pImageData = st_new_array<st_uchar>(nImageSize, "CTgaFormat::st_uchar");

                    st_int32 nPixels = m_nWidth * m_nHeight;
                    st_int32 nCurrentPixel = 0;
                    st_int32 nCurrentByte = 0;
                    st_uchar* pBuffer = st_new_array<st_uchar>(m_nDepth, "CTgaFormat::st_uchar");

                    do
                    {
                        GLubyte ubChunkHeader = 0; // Variable To Store The Value Of The Id Chunk
                        fread((char*)&ubChunkHeader, sizeof(GLubyte), 1, pFile);
                
                        if (ubChunkHeader < 128) // If The Chunk Is A 'RAW' Chunk
                        {
                            ++ubChunkHeader; // Add 1 To The Value To Get Total Number Of Raw Pixels

                            for (st_int32 i = 0; i < ubChunkHeader; ++i)
                            {
                                fread((char*)pBuffer, m_nDepth * sizeof(char), 1, pFile);
                    
                                m_pImageData[nCurrentByte] = pBuffer[0];
                                m_pImageData[nCurrentByte + 1] = pBuffer[1];
                                m_pImageData[nCurrentByte + 2] = pBuffer[2];
                                if (m_nDepth == 4)
                                    m_pImageData[nCurrentByte + 3] = pBuffer[3];

                                nCurrentByte += m_nDepth;
                                ++nCurrentPixel;
                            }
                        }
                        else
                        {
                            ubChunkHeader -= 127; // Subtract 127 To Get Rid Of The ID Bit

                            fread((char*)pBuffer, m_nDepth * sizeof(char), 1, pFile);
                                    
                            for (st_int32 i = 0; i < ubChunkHeader; ++i)
                            {
                                m_pImageData[nCurrentByte] = pBuffer[0];
                                m_pImageData[nCurrentByte + 1] = pBuffer[1];
                                m_pImageData[nCurrentByte + 2] = pBuffer[2];
                                if (m_nDepth == 4)
                                    m_pImageData[nCurrentByte + 3] = pBuffer[3];

                                nCurrentByte += m_nDepth;
                                ++nCurrentPixel;
                            }
                        }
                    } 
                    while (nCurrentPixel < nPixels);

                    st_delete_array<st_uchar>(pBuffer);
                    bSuccess = true;
                }
            }
        }
        fclose(pFile);

        if (bFlip && m_pImageData != NULL)
        {
            st_uint32 uiRowSize = m_nWidth * m_nDepth;
            st_uchar* pTemp = st_new_array<st_uchar>(uiRowSize, "CTgaFormat::st_uchar");

            #ifdef ST_OPENMP
                #pragma omp parallel for
            #endif
            for (st_int32 i = 0; i < (int)m_nHeight / 2; ++i)
            {
                memcpy(pTemp, &m_pImageData[i * uiRowSize], uiRowSize);
                memcpy(&m_pImageData[i * uiRowSize], &m_pImageData[(m_nHeight - i - 1) * uiRowSize], uiRowSize);
                memcpy(&m_pImageData[(m_nHeight - i - 1) * uiRowSize], pTemp, uiRowSize);
            }

            st_delete_array<st_uchar>(pTemp);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::GetPixel

st_uchar* CTgaFormat::GetPixel(st_int32 nWidth, st_int32 nHeight) const
{
    st_uchar* pPixel = NULL;

    if (m_pImageData)
        pPixel = m_pImageData + m_nDepth * (m_nWidth * nHeight + nWidth);

    return pPixel;
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::SetPixel

void CTgaFormat::SetPixel(const st_uchar* pValue, st_int32 nWidth, st_int32 nHeight)
{
    if (m_pImageData)
        memcpy(m_pImageData + m_nDepth * (m_nWidth * nHeight + nWidth), pValue, m_nDepth);
}


///////////////////////////////////////////////////////////////////////  
//  CTgaFormat::CreateOpenGLTexture

GLuint CTgaFormat::CreateOpenGLTexture(st_int32 nMaxAnisotropy) const
{
    GLuint uiTexture = 0;

    if (m_pImageData)
    {
        glGenTextures(1, &uiTexture);
        
        if (uiTexture > 0)
        {
            glBindTexture(GL_TEXTURE_2D, uiTexture);

            if (nMaxAnisotropy > 0)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, nMaxAnisotropy);
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            (void) gluBuild2DMipmaps(GL_TEXTURE_2D, m_nDepth, m_nWidth, m_nHeight, m_nDepth == 3 ? GL_BGR : GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_pImageData);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    return uiTexture;
}




