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
//  Static member variables

LPDIRECT3DDEVICE9 DX9::m_pDx9 = NULL;
CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache = NULL;
CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache = NULL;
CTexture::CTextureCache* CTexture::m_pCache = NULL;
CStateBlock::CStateBlockCache* CStateBlock::m_pCache = NULL;

CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
st_int32 CRenderState::m_nFallbackTextureRefCount = 0;

LPDIRECT3DSURFACE9 CRenderTargetDirectX9::m_pPreviouslyActiveRenderTargetSurface = NULL;
LPDIRECT3DSURFACE9 CRenderTargetDirectX9::m_pPreviouslyActiveDepthStencilSurface = NULL;
D3DVIEWPORT9 CRenderTargetDirectX9::m_sPreviouslyActiveViewport = D3DVIEWPORT9( );


///////////////////////////////////////////////////////////////////////
//  DX9::Device

LPDIRECT3DDEVICE9 DX9::Device(void)                         
{ 
    return m_pDx9; 
}


///////////////////////////////////////////////////////////////////////
//  DX9::SetDevice

void DX9::SetDevice(LPDIRECT3DDEVICE9 pDevice)  
{ 
    m_pDx9 = pDevice; 
}


