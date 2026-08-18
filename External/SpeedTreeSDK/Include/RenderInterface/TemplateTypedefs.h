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
//  Class CVisibleInstancesRI

typedef CVisibleInstancesRI<CStateBlock,
                            CTexture, 
                            CGeometryBuffer,
                            CInstancingMgr,
                            CShaderTechnique,
                            CShaderConstantBuffer> CVisibleInstancesRender;


///////////////////////////////////////////////////////////////////////  
//  Class CForestRender

typedef CForestRI<CStateBlock,
                  CTexture, 
                  CGeometryBuffer,
                  CInstancingMgr,
                  CShaderTechnique,
                  CShaderConstantBuffer> CForestRender;


///////////////////////////////////////////////////////////////////////  
//  Class CRenderState

typedef CRenderStateRI<CStateBlock,
                       CTexture, 
                       CShaderTechnique,
                       CShaderConstantBuffer> CRenderState; 


///////////////////////////////////////////////////////////////////////  
//  Class CTreeRender

typedef CTreeRI<CStateBlock,
                CTexture, 
                CGeometryBuffer,
                CShaderTechnique,
                CShaderConstantBuffer> CTreeRender;


///////////////////////////////////////////////////////////////////////  
//  Class CTerrainRender

typedef CTerrainRI<CStateBlock,
                   CTexture, 
                   CGeometryBuffer, 
                   CShaderTechnique,
                   CShaderConstantBuffer> CTerrainRender;


///////////////////////////////////////////////////////////////////////  
//  Class CSkyRender

typedef CSkyRI<CStateBlock,
               CTexture,
               CGeometryBuffer, 
               CShaderTechnique, 
               CShaderConstantBuffer> CSkyRender;


///////////////////////////////////////////////////////////////////////  
//  App-side constant buffers

typedef SFrameConstBuf<CShaderConstantBuffer, CTexture> SFrameConstBufApp;
typedef SBloomConstBuf<CShaderConstantBuffer, CTexture> SBloomConstBufApp;


///////////////////////////////////////////////////////////////////////  
//  Explicit static member template specialization declarations and exports

#if defined(ST_USE_SDK_AS_DLLS) || defined(ST_NEEDS_EXPLICIT_TEMPLATE_DECL)
    template<> ST_DLL_LINK CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache;
    template<> ST_DLL_LINK CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache;
    template<> ST_DLL_LINK CTexture::CTextureCache* CTexture::m_pCache;
    template<> ST_DLL_LINK CStateBlock::CStateBlockCache* CStateBlock::m_pCache;
    template<> ST_DLL_LINK CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS];
    template<> ST_DLL_LINK CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS];
    template<> ST_DLL_LINK st_int32 CRenderState::m_nFallbackTextureRefCount;
#endif
