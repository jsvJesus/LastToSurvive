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

#include "Renderers/OpenGL/OpenGLRenderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  Static member variables

template<> CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache = NULL;
template<> CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache = NULL;
template<> CTexture::CTextureCache* CTexture::m_pCache = NULL;
template<> CStateBlock::CStateBlockCache* CStateBlock::m_pCache = NULL;

template<> CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> st_int32 CRenderState::m_nFallbackTextureRefCount = 0;

