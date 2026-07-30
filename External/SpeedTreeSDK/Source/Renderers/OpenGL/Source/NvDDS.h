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


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/Types.h"
#include "Renderers/OpenGL/OpenGLRenderer.h"


///////////////////////////////////////////////////////////////////////  
//  DDS Info

#define DDS_FOURCC  0x00000004
#define DDS_RGB     0x00000040
#define DDS_RGBA    0x00000041
#define DDS_DEPTH   0x00800000

#define DDS_COMPLEX 0x00000008
#define DDS_CUBEMAP 0x00000200
#define DDS_VOLUME  0x00200000

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    ///////////////////////////////////////////////////////////////////////  
    //  class CNvDDS

    class CNvDDS
    {
    public:
            struct SDDSPixelFormat
            {
                st_uint32   m_uiSize;
                st_uint32   m_uiFlags;
                st_uint32   m_uiFourCC;
                st_uint32   m_uiRGBBitCount;
                st_uint32   m_uiRBitMask;
                st_uint32   m_uiGBitMask;
                st_uint32   m_uiBBitMask;
                st_uint32   m_uiABitMask;
            };

            struct SDXTColBlock
            {
                st_uint16   m_usCol0;
                st_uint16   m_usCol1;
                st_uint8    m_aucRow[4];
            };

            struct SDXT3AlphaBlock
            {
                st_uint16   m_ausRow[4];
            };

            struct SDXT5AlphaBlock
            {
                st_uint8    m_ucAlpha0;
                st_uint8    m_ucAlpha1;
                st_uint8    m_aucRow[6];
            };

            struct SDDSHeader
            {
                st_uint32       m_uiSize;
                st_uint32       m_uiFlags;
                st_uint32       m_uiHeight;
                st_uint32       m_uiWidth;
                st_uint32       m_uiPitchOrLinearSize;
                st_uint32       m_uiDepth;
                st_uint32       m_uiMipMapCount;
                st_uint32       m_auiReserved1[11];
                SDDSPixelFormat m_sPixelFormat;
                st_uint32       m_uiCaps1;
                st_uint32       m_uiCaps2;
                st_uint32       m_auiReserved2[3];
            };

            // DDS files are loaded upside down by default (faster), set bFlip = true to flip the texture upright (slower)
            static  GLuint  LoadFromFile(const char* pFilename, 
                                         SpeedTree::STextureInfo& sInfo, 
                                         GLenum eMin, 
                                         GLenum eMag,
                                         SDDSHeader& sFileHeader,
                                         SpeedTree::st_bool bFlip = false, 
                                         SpeedTree::st_int32 nMaxAnisotropy = 0);

            static  GLuint  LoadFromData(const char* pFilename, 
                                         SpeedTree::STextureInfo& sInfo, 
                                         unsigned char* pData, 
                                         size_t siNumBytes, 
                                         GLenum eMin, 
                                         GLenum eMag,
                                         SDDSHeader& sDDSHeader,
                                         SpeedTree::st_bool bFlip = false, 
                                         SpeedTree::st_int32 nMaxAnisotropy = 0);
    };

} // end namespace SpeedTree

