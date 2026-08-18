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

#pragma once
#include "Renderers/OpenGL/OpenGLRenderer.h"


///////////////////////////////////////////////////////////////////////  
//  Class CTgaFormat

class CTgaFormat
{
public:
                                CTgaFormat( );
                                CTgaFormat(SpeedTree::st_uchar* pImageData, SpeedTree::st_int32 nWidth, SpeedTree::st_int32 nHeight, SpeedTree::st_int32 nDepth, SpeedTree::st_bool bCopyData = true);
                                ~CTgaFormat( );

        SpeedTree::st_bool      Read(const char* pFilename, SpeedTree::st_bool bFlip = false);

        SpeedTree::st_int32     GetWidth(void) const            { return m_nWidth; }
        SpeedTree::st_int32     GetHeight(void) const           { return m_nHeight; }
        SpeedTree::st_int32     GetDepth(void) const            { return m_nDepth; } // in bytes (3 = RGB, 4 = RGBA)

        SpeedTree::st_uchar*    GetPixel(SpeedTree::st_int32 nWidth, SpeedTree::st_int32 nHeight) const;
        void                    SetPixel(const SpeedTree::st_uchar* pValue, SpeedTree::st_int32 nWidth, SpeedTree::st_int32 nHeight);

        // raw data format is either RGB or RGBA, one byte per color component
        SpeedTree::st_uchar*    GetRawData(void) const          { return m_pImageData; }
    
        GLuint                  CreateOpenGLTexture(SpeedTree::st_int32 nMaxAnisotropy = 0) const;

private:
        SpeedTree::st_int32     m_nHeight;          // in pixels
        SpeedTree::st_int32     m_nWidth;           // in pixels
        SpeedTree::st_int32     m_nDepth;           // in bytes (3 = RGB, 4 = RGBA)
        SpeedTree::st_uchar*    m_pImageData;       // raw data in GL_ABGR_EXT format
        SpeedTree::st_bool      m_bDeleteData;      // flag - make a copy of the data passed in from 2nd constructor?
};


