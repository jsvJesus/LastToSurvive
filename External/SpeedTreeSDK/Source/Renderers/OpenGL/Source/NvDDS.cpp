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
//      Web: http://www.idvinc.com

// Most of this code is taken from nv_dds.h/.cpp available from NVidia


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#include "NvDDS.h"
#include "Core/Array.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Endian Issues

//#ifdef BIG_ENDIAN
//  #define EndianSwap32(a) (((((a) & (st_uint32) 0xff000000) >> 24) | ((a) & (st_uint32) 0x00ff0000) >> 8 | ((a) & (st_uint32) 0x0000ff00) << 8 | ((a) & (st_uint32) 0x000000ff) << 24))
//  #define EndianSwap16(a) (((((a) & 0xff00) >> 8) | (((a) & 0x00ff) << 8)))
//#else
    #define EndianSwap32
    #define EndianSwap16
//#endif


///////////////////////////////////////////////////////////////////////
// FlipDXT1Block

static void FlipDXT1Block(CNvDDS::SDXTColBlock* pLine, st_int32 iNumBlocks)
{
    CNvDDS::SDXTColBlock* pCurrentBlock = pLine;
    st_uchar ucTemp;

    for (st_int32 i = 0; i < iNumBlocks; ++i)
    {
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////
// FlipDXT3Block

static void FlipDXT3Block(CNvDDS::SDXTColBlock* pLine, st_int32 iNumBlocks)
{
    CNvDDS::SDXTColBlock* pCurrentBlock = pLine;
    CNvDDS::SDXT3AlphaBlock* pAlphaBlock = NULL;

    st_uchar ucTemp;
    st_uint16 usTemp;

    for (st_int32 i = 0; i < iNumBlocks; ++i)
    {
        // alpha
        pAlphaBlock = (CNvDDS::SDXT3AlphaBlock*) pCurrentBlock;

        usTemp = pAlphaBlock->m_ausRow[0];
        pAlphaBlock->m_ausRow[0] = pAlphaBlock->m_ausRow[3];
        pAlphaBlock->m_ausRow[3] = usTemp;

        usTemp = pAlphaBlock->m_ausRow[1];
        pAlphaBlock->m_ausRow[1] = pAlphaBlock->m_ausRow[2];
        pAlphaBlock->m_ausRow[2] = usTemp;

        pCurrentBlock++;


        // color
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////
// FlipDXT5Block

static void FlipDXT5Block(CNvDDS::SDXTColBlock* pLine, st_int32 iNumBlocks)
{
    CNvDDS::SDXTColBlock* pCurrentBlock = pLine;
    CNvDDS::SDXT5AlphaBlock *pAlphaBlock = NULL;

    st_uchar ucTemp;
    const st_uint32 c_ulMask = 0x00000007; // ulBits = 00 00 01 11
    st_uint32 ulBits = 0;
    st_uchar aucBits[4][4];
    
    for (st_int32 i = 0; i < iNumBlocks; ++i)
    {
        // alpha
        pAlphaBlock = (CNvDDS::SDXT5AlphaBlock*) pCurrentBlock;
        
        memcpy(&ulBits, &pAlphaBlock->m_aucRow[0], sizeof(st_uchar) * 3);
        
        aucBits[0][0] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][1] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][2] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[0][3] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][0] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][1] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][2] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[1][3] = (st_uchar)(ulBits & c_ulMask);

        ulBits = 0;
        memcpy(&ulBits, &pAlphaBlock->m_aucRow[3], sizeof(st_uchar) * 3);

        aucBits[2][0] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][1] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][2] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[2][3] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][0] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][1] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][2] = (st_uchar)(ulBits & c_ulMask);
        ulBits >>= 3;
        aucBits[3][3] = (st_uchar)(ulBits & c_ulMask);

        st_uint32 *pBits = ((st_uint32*) &(pAlphaBlock->m_aucRow[0]));
        *pBits &= 0xff000000;

        *pBits = *pBits | (aucBits[3][0] << 0);
        *pBits = *pBits | (aucBits[3][1] << 3);
        *pBits = *pBits | (aucBits[3][2] << 6);
        *pBits = *pBits | (aucBits[3][3] << 9);

        *pBits = *pBits | (aucBits[2][0] << 12);
        *pBits = *pBits | (aucBits[2][1] << 15);
        *pBits = *pBits | (aucBits[2][2] << 18);
        *pBits = *pBits | (aucBits[2][3] << 21);

        pBits = ((st_uint32*)(&(pAlphaBlock->m_aucRow[3])));
        *pBits &= 0xff000000;

        *pBits = *pBits | (aucBits[1][0] << 0);
        *pBits = *pBits | (aucBits[1][1] << 3);
        *pBits = *pBits | (aucBits[1][2] << 6);
        *pBits = *pBits | (aucBits[1][3] << 9);

        *pBits = *pBits | (aucBits[0][0] << 12);
        *pBits = *pBits | (aucBits[0][1] << 15);
        *pBits = *pBits | (aucBits[0][2] << 18);
        *pBits = *pBits | (aucBits[0][3] << 21);

        pCurrentBlock++;

        // color
        ucTemp = pCurrentBlock->m_aucRow[0];
        pCurrentBlock->m_aucRow[0] = pCurrentBlock->m_aucRow[3];
        pCurrentBlock->m_aucRow[3] = ucTemp;

        ucTemp = pCurrentBlock->m_aucRow[1];
        pCurrentBlock->m_aucRow[1] = pCurrentBlock->m_aucRow[2];
        pCurrentBlock->m_aucRow[2] = ucTemp;

        pCurrentBlock++;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CNvDDS::LoadFromFile

GLuint CNvDDS::LoadFromFile(const char* pFilename, STextureInfo& sInfo, GLenum eMin, GLenum eMag, SDDSHeader& sFileHeader,st_bool bFlip, st_int32 nMaxAnisotropy)
{
    st_uint32 uiReturn = 0;

    CFixedString strFilename(pFilename);
    if (!strFilename.empty( ))
    {
        FILE* pFile = fopen(strFilename.c_str( ), "rb");
        if (pFile != NULL)
        {   
            fseek(pFile, 0L, SEEK_END);
            st_int32 nNumBytes = st_int32(ftell(pFile));
            st_int32 nErrorCode = fseek(pFile, 0, SEEK_SET);
            
            if (nNumBytes > 0 && nErrorCode >= 0)
            {
                CStaticArray<st_byte> aData(nNumBytes, "CNvDDS::st_byte");
                st_int32 nNumBytesRead = int(fread(&aData[0], 1, nNumBytes, pFile));
                if (nNumBytesRead == nNumBytes)
                    // pFilename is passed in so warnings can be issued by filename
                    uiReturn = LoadFromData(pFilename, sInfo, &aData[0], nNumBytes, eMin, eMag, sFileHeader, bFlip, nMaxAnisotropy);
            }

            fclose(pFile);
        }
    }

    return uiReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CNvDDS::LoadFromData
//
//  Returns 0 if unsuccessful

GLuint CNvDDS::LoadFromData(const char* pFilename, 
                            STextureInfo& sInfo, 
                            st_uchar* pchData, 
                            size_t siNumBytes, 
                            GLenum eMin, 
                            GLenum eMag,
                            SDDSHeader& sFileHeader,
                            st_bool bFlip, 
                            st_int32 nMaxAnisotropy)
{
    st_uint32 uiReturn = 0;

    if (siNumBytes > 0 && pchData != NULL)
    {
        if (pchData[0] == 'D' && pchData[1] == 'D' && pchData[2] == 'S' && pchData[3] == ' ')
        {
            memcpy(&sFileHeader, &(pchData[4]), sizeof(SDDSHeader));
            
            st_int32 nWidth = sInfo.m_nWidth = EndianSwap32(sFileHeader.m_uiWidth);
            st_int32 nHeight = sInfo.m_nHeight = EndianSwap32(sFileHeader.m_uiHeight);

            st_uint32 uiDataIndex = sizeof(SDDSHeader) + 4;
            st_int32 nNumMipmaps = EndianSwap32(sFileHeader.m_uiMipMapCount);
            if (nNumMipmaps == 0)
            {
                nNumMipmaps = 1;
                CCore::SetError("DDS file [%s] has only one mipmap; this DDS loader will not automatically generate mipmaps; mipmap filtering disabled for this texture", pFilename);
            }

            st_int32 iFormatFlags = EndianSwap32(sFileHeader.m_sPixelFormat.m_uiFlags);

            if (iFormatFlags & DDS_FOURCC)
            {
                // DXT Compressed textures

                st_int32 iFormat = 0;
                st_int32 iBlockSize = 0;
                void (*fpFlipBlocks)(SDXTColBlock*, int) = NULL;

                switch(EndianSwap32(sFileHeader.m_sPixelFormat.m_uiFourCC))
                {
                case FOURCC_DXT1:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    iBlockSize = 8;
                    fpFlipBlocks = &FlipDXT1Block;
                    break;
                case FOURCC_DXT3:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                    iBlockSize = 16;
                    fpFlipBlocks = &FlipDXT3Block;
                    break;
                case FOURCC_DXT5:
                    iFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    iBlockSize = 16;
                    fpFlipBlocks = &FlipDXT5Block;
                    break;
                default:
                    break;
                }

                if (iFormat != 0)
                {
                    glGenTextures(1, &uiReturn);
                    //glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, uiReturn);
                    
                    if (nMaxAnisotropy > 0)
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, st_float32(nMaxAnisotropy));

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (nNumMipmaps == 1 ? GL_LINEAR : eMin));
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (nNumMipmaps == 1 ? GL_LINEAR : eMag));

                    for (st_int32 i = 0; i < nNumMipmaps && (nWidth || nHeight); i++)
                    {
                        st_int32 iSize = ((nWidth + 3) / 4) * ((nHeight + 3) / 4) * (iFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
                        st_uchar* pData = &pchData[uiDataIndex];

                        if (bFlip)
                        {
                            st_int32 iXBlocks = nWidth / 4;
                            st_int32 iYBlocks = nHeight / 4;
                            st_int32 iLineSize = iXBlocks * iBlockSize;

                            st_uchar* pTempBlock = st_new_array<st_uchar>(iLineSize, "CNvDDS::st_uchar");

                            #ifdef ST_OPENMP
                                #pragma omp parallel for
                            #endif
                            for (st_int32 j = 0; j < (iYBlocks >> 1); ++j)
                            {
                                SDXTColBlock* pTop = reinterpret_cast<SDXTColBlock*>(pData + j * iLineSize);
                                SDXTColBlock* pBottom = reinterpret_cast<SDXTColBlock*>(pData + ((iYBlocks - j - 1) * iLineSize));

                                (*fpFlipBlocks)(pTop, iXBlocks);
                                (*fpFlipBlocks)(pBottom, iXBlocks);

                                memcpy(pTempBlock, pBottom, iLineSize);
                                memcpy(pBottom, pTop, iLineSize);
                                memcpy(pTop, pTempBlock, iLineSize);
                            }

                            st_delete_array<st_uchar>(pTempBlock);
                        }

                        glCompressedTexImage2D(GL_TEXTURE_2D, i, iFormat, nWidth, nHeight, 0, iSize, pData);
                        uiDataIndex += iSize;
                        if (st_int32(uiDataIndex) > siNumBytes)
                        {
                            glDeleteTextures(1, &uiReturn);
                            return 0;
                        }

                        nWidth = nWidth >> 1;
                        if (nWidth <= 0)
                            nWidth = 1;
                        nHeight = nHeight >> 1;
                        if (nHeight <= 0)
                            nHeight = 1; 
                    }               
                }
            }
            else if ((iFormatFlags & DDS_RGB) || (iFormatFlags & DDS_RGBA))
            {
                // uncompressed RGB and RGBA textures

                glGenTextures(1, &uiReturn);
                //glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, uiReturn);
                
                if (nMaxAnisotropy > 0)
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, st_float32(nMaxAnisotropy));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (nNumMipmaps == 1 ? GL_LINEAR : eMin));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (nNumMipmaps == 1 ? GL_LINEAR : eMag));

                st_int32 iPixelSize = EndianSwap32(sFileHeader.m_sPixelFormat.m_uiRGBBitCount);
                st_int32 iFormat = (iPixelSize == 24) ? GL_RGB : GL_RGBA;

                for (st_int32 i = 0; i < nNumMipmaps && (nWidth || nHeight); i++)
                {
                    st_uchar* pData = &pchData[uiDataIndex];

                    glTexImage2D(GL_TEXTURE_2D, i, iFormat, nWidth, nHeight, 0, iFormat, GL_UNSIGNED_BYTE, pData);
                    
                    uiDataIndex += nWidth * nHeight * iPixelSize / 8;
                    if (st_int32(uiDataIndex) > siNumBytes)
                    {
                        glDeleteTextures(1, &uiReturn);
                        return 0;
                    }

                    nWidth = nWidth >> 1;
                    if (nWidth <= 0)
                        nWidth = 1;
                    nHeight = nHeight >> 1;
                    if (nHeight <= 0)
                        nHeight = 1; 
                }               
            }
        }
    }

    return uiReturn;
}
