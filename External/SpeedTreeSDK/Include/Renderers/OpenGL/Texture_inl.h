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
//  CTextureOpenGL::CTextureOpenGL

inline CTextureOpenGL::CTextureOpenGL( ) :
    m_uiTexture(0)
{
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::~CTextureOpenGL

inline CTextureOpenGL::~CTextureOpenGL( )
{
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::IsValid

inline st_bool CTextureOpenGL::IsValid(void) const
{
    return (m_uiTexture != 0u);
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::GetTextureObject

inline st_uint32 CTextureOpenGL::GetTextureObject(void) const
{
    return m_uiTexture;
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::SetSamplerStates

inline void CTextureOpenGL::SetSamplerStates(void)
{
    // intentionally empty
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::CreateSharedSamplers

inline void CTextureOpenGL::CreateSharedSamplers(st_int32 nMaxAnisotropy)
{
    ST_UNREF_PARAM(nMaxAnisotropy);

    // intentionally empty
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::GetInfo

inline const STextureInfo& CTextureOpenGL::GetInfo(void) const
{
    return m_sInfo;
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::operator=

inline CTextureOpenGL& CTextureOpenGL::operator=(const CTextureOpenGL& cRight)
{
    m_uiTexture = cRight.m_uiTexture;

    return *this;
}


///////////////////////////////////////////////////////////////////////
//  CTextureOpenGL::operator!=

inline st_bool CTextureOpenGL::operator!=(const CTextureOpenGL& cRight) const
{
    return (m_uiTexture != cRight.m_uiTexture);
}
