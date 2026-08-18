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
//  CForestRI::CForestRI

CForestRI_TemplateList
inline CForestRI_t::CForestRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::~CForestRI

CForestRI_TemplateList
inline CForestRI_t::~CForestRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::ReleaseGfxResources

CForestRI_TemplateList
inline void CForestRI_t::ReleaseGfxResources(void)
{
    m_tFizzleNoise.ReleaseGfxResources( );
    m_tPerlinNoiseKernel.ReleaseGfxResources( );
    m_tAmbientImageLighting.ReleaseGfxResources( );
    m_sFrameConstantBuffer.ReleaseGfxResources( );
    m_sFogAndSkyConstantBuffer.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::SetRenderInfo

CForestRI_TemplateList
inline void CForestRI_t::SetRenderInfo(const SForestRenderInfo& sInfo)
{
    m_sRenderInfo = sInfo;

    // fog/sky parameters (fog color can blend into sky color)
    (void) UpdateFogAndSkyConstantBuffer( );
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::GetRenderInfo

CForestRI_TemplateList
inline const SForestRenderInfo& CForestRI_t::GetRenderInfo(void) const
{
    return m_sRenderInfo;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::InitGfx

CForestRI_TemplateList
inline st_bool CForestRI_t::InitGfx(void)
{
    st_bool bSuccess = false;

    // texture samplers
    TTextureClass::CreateSharedSamplers(m_sRenderInfo.m_nMaxAnisotropy);

    // alpha noise texture (or white if off)
    if (m_sRenderInfo.m_sAppState.m_bAlphaToCoverage)
        bSuccess = m_tFizzleNoise.LoadColor(0xffff33ff);
    else
        bSuccess = m_tFizzleNoise.LoadNoise(c_nNoiseTexWidth, c_nNoiseTexWidth);

    // noise
    bSuccess &= m_tPerlinNoiseKernel.LoadPerlinNoiseKernel(256, 256, 4);

    // image-based ambient lighting
    if (!m_sRenderInfo.m_strImageBasedAmbientLightingFilename.empty( ))
    {
        if (!m_tAmbientImageLighting.Load(m_sRenderInfo.m_strImageBasedAmbientLightingFilename.c_str( )))
        {
            CCore::SetError("Failed to load image based ambient lighting texture [%s]", m_sRenderInfo.m_strImageBasedAmbientLightingFilename.c_str( ));
            bSuccess = false;
        }
    }

    // constant buffer
    bSuccess &= m_sFrameConstantBuffer.Init(sizeof(m_sFrameConstantBuffer), ST_CONST_BUF_REGISTER_FRAME);
    bSuccess &= m_sFogAndSkyConstantBuffer.Init(sizeof(m_sFogAndSkyConstantBuffer), ST_CONST_BUF_REGISTER_FOG_AND_SKY);

    // set a reserve capacity for the draw calls array to avoid allocations during render loop
    m_aSortedDrawCalls.reserve(300);
    
    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::StartRender

CForestRI_TemplateList
inline st_bool CForestRI_t::StartRender(void)
{
    st_bool bSuccess = true;

    // set some of the shader constants that are shared by all shaders, regardless of configuration; start
    // by grabbing any one of the trees registered to the forest and use
    {
        // set up samplers
        TTextureClass::SetSamplerStates( );

        CRenderStateRI_t::ClearLastBoundTextures( );

        // shader constant buffers
        bSuccess &= m_sFrameConstantBuffer.Bind( );
        bSuccess &= m_sFogAndSkyConstantBuffer.Bind( );

        // bind noise textures
        bSuccess &= TShaderConstantBufferClass::SetTexture(TEXTURE_REGISTER_FIZZLE_NOISE, m_tFizzleNoise, false);
        bSuccess &= TShaderConstantBufferClass::SetTexture(TEXTURE_REGISTER_PERLIN_NOISE_KERNEL, m_tPerlinNoiseKernel, false);
        bSuccess &= TShaderConstantBufferClass::SetTexture(0, m_tPerlinNoiseKernel, false);

        bSuccess &= TShaderConstantBufferClass::SetTexture(TEXTURE_REGISTER_IMAGE_BASED_AMBIENT_LIGHTING, m_tAmbientImageLighting, false);
    }

    if (!bSuccess)
        CCore::SetError("CForestRI::StartRender() failed to set one or more shader constants");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::EndRender

CForestRI_TemplateList
inline st_bool CForestRI_t::EndRender(void)
{
    st_bool bSuccess = true;

    bSuccess &= CRenderStateRI_t::UnBind( );
    bSuccess &= TGeometryBufferClass::UnBindVertexBuffer( );
    bSuccess &= TGeometryBufferClass::UnBindIndexBuffer( );
    bSuccess &= TShaderTechniqueClass::UnBind( );

    CForest::FrameEnd( );

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::Render3dTrees

CForestRI_TemplateList
inline st_bool CForestRI_t::Render3dTrees(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances) const
{
    ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::Render3dTrees(Lit)" : "CForestRI_t::Render3dTrees(Depth-only)");

    st_bool bSuccess = true;

    // clear sorted draw call array
    m_aSortedDrawCalls.resize(0);

    // queue draw calls, but don't execute them yet
    {
        const TDetailedCullDataArray& aPerBase3dInstances = cVisibleInstances.Get3dInstanceLods( );
        for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
        {
            // get base tree pointer
            CTreeRI_t* pBaseTree = (CTreeRI_t*) aPerBase3dInstances[nBase].m_pBaseTree;
            assert(pBaseTree);

            // get this base tree's instance list
            const T3dTreeInstanceLodArray* pInstanceLods = &aPerBase3dInstances[nBase].m_a3dInstanceLods;
            if (pInstanceLods->empty( ))
                continue;

            // grab the instancing data for this base tree
            const typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = cVisibleInstances.FindInstancingDataByBaseTree(pBaseTree);
            if (pInstancingData)
            {
                // stats tracking
                CRenderStats::SObjectStats& sObjStats = m_cRenderStats.GetObjectStats(pBaseTree->GetFilename( ));
                sObjStats.m_nNumInstances += st_int32(pInstanceLods->size( ));

                // access geometry
                const SGeometry* pGeometry = pBaseTree->GetGeometry( );
                assert(pGeometry);
                assert(static_cast<const SLod*>(pGeometry->m_pLods));

                // run through each possible LOD level, looking for instances that should be rendered at that LOD
                for (st_int32 nLod = 0; nLod < pGeometry->m_nNumLods; ++nLod)
                {
                    // if there's nothing available at this LOD, move to the next
                    if (pInstancingData->m_t3dTreeInstancingMgr.NumInstances(nLod) < 1)
                        continue;

                    const SLod* pLod = pGeometry->m_pLods + nLod;
                    assert(pLod);

                    // each instance may require several draw calls to render
                    assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
                    for (st_int32 nDrawCall = 0; nDrawCall < pLod->m_nNumDrawCalls; ++nDrawCall)
                    {
                        // geometry buffer
                        const TGeometryBufferClass* pGeometryBuffer = pBaseTree->GetGeometryBuffer(nLod, nDrawCall);
                        assert(pGeometryBuffer);

                        // draw call
                        const SDrawCall* pDrawCall = pLod->m_pDrawCalls + nDrawCall;
                        if (pDrawCall->m_nNumVertices > 0 && pDrawCall->m_nNumIndices > 0)
                        {
                            // render state
                            const CRenderStateRI_t* pRenderState = &pBaseTree->Get3dRenderStates(ePass)[pDrawCall->m_nRenderStateIndex];

                            // skip this object if we're in a shadow-casting pass and this object is flagged as non-caster
                            if (ePass == RENDER_PASS_SHADOW_CAST && !pRenderState->m_bCastsShadows)
                                continue;

                            // passed all conditions for skipping the render, so add it to the draw call queue
                            SDrawCallData sDraw;
                            sDraw.m_pBaseTree = pBaseTree;
                            sDraw.m_nLod = nLod;
                            sDraw.m_pRenderState = pRenderState;
                            sDraw.m_pDrawCall = pDrawCall;
                            sDraw.m_pGeometryBuffer = pGeometryBuffer;
                            sDraw.m_nBufferOffset = pBaseTree->GetGeometryBufferOffset(nLod, nDrawCall);
                            sDraw.m_pInstancingData = pInstancingData;
                            sDraw.m_pStats = &sObjStats;
                            m_aSortedDrawCalls.insert_sorted(sDraw);
                        }
                    }
                }
            }
        }
    }

    // execute draw calls
    st_int64 lCurrentSortKey = -1;
    for (int nDrawCall = 0; nDrawCall < st_int32(m_aSortedDrawCalls.size( )); ++nDrawCall)
    {
        const SDrawCallData& sDraw = m_aSortedDrawCalls[nDrawCall];

        (void) sDraw.m_pBaseTree->BindConstantBuffers( );
        (void) sDraw.m_pRenderState->BindConstantBuffer( );
        (void) sDraw.m_pRenderState->BindTextures(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK);

        if (lCurrentSortKey != sDraw.GetHashKey( ))
        {
            sDraw.m_pRenderState->BindShader( );
            sDraw.m_pRenderState->BindStateBlock( );

            lCurrentSortKey = sDraw.GetHashKey( );
        }

        SInstancedDrawStats sInstanceStats;
        bSuccess &= sDraw.m_pInstancingData->m_t3dTreeInstancingMgr.Render3dTrees(sDraw.m_nBufferOffset, sDraw.m_nLod, sInstanceStats);

        // update render statistics
        assert(sDraw.m_pStats);
        sDraw.m_pStats->m_nNumDrawCalls += sInstanceStats.m_nNumDrawCalls;
        st_int32 nNumTriangles = st_int32(sDraw.m_pGeometryBuffer->NumIndices( ) / 3) * sInstanceStats.m_nNumInstancesDrawn;
        sDraw.m_pStats->m_nNumTrianglesRendered += nNumTriangles;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::RenderGrass

CForestRI_TemplateList
inline st_bool CForestRI_t::RenderGrass(ERenderPass ePass, const CTreeRI_t* pBaseGrass, const CVisibleInstancesRI_t& cVisibleInstances) const
{
    ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::RenderGrass(Lit)" : "CForestRI_t::RenderGrass(Depth-only)");

    st_bool bSuccess = true;

    // grab the instancing data for this base tree
    const typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = cVisibleInstances.FindInstancingDataByBaseTree(pBaseGrass);
    if (pInstancingData && pInstancingData->m_t3dTreeInstancingMgr.NumInstances( ) > 0)
    {
        // access geometry
        const SGeometry* pGeometry = pBaseGrass->GetGeometry( );
        assert(pGeometry);
        assert(static_cast<const SLod*>(pGeometry->m_pLods));

        // always render the highest LOD (should really only be one for grass models anyway)
        const SLod* pLod = pGeometry->m_pLods + 0;
        assert(pLod);

        // stats tracking
        CRenderStats::SObjectStats& sObjStats = m_cRenderStats.GetObjectStats(pBaseGrass->GetFilename( ));

        // for grass, we expect at most one draw call
        assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
        if (pLod->m_nNumDrawCalls > 0)
        {
            const SDrawCall* pDrawCall = pLod->m_pDrawCalls + 0;
            if (pDrawCall->m_nNumVertices > 0 && pDrawCall->m_nNumIndices > 0)
            {
                // geometry buffer
                const TGeometryBufferClass* pGeometryBuffer = pBaseGrass->GetGeometryBuffer(0, 0);
                assert(pGeometryBuffer);

                // render state
                const CRenderStateRI_t* pRenderState = &pBaseGrass->Get3dRenderStates(ePass)[pDrawCall->m_nRenderStateIndex];

                if (pBaseGrass->BindConstantBuffers( ) &&
                    pRenderState->BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK))
                {
                    st_int32 nBufferOffset = pBaseGrass->GetGeometryBufferOffset(0, 0);

                    SInstancedDrawStats sInstanceStats;
                    bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.RenderGrass(nBufferOffset, sInstanceStats);

                    // update render statistics
                    sObjStats.m_nNumDrawCalls += 1;
                    st_int32 nNumTriangles = st_int32(pGeometryBuffer->NumIndices( ) / 3) * sInstanceStats.m_nNumInstancesDrawn;
                    sObjStats.m_nNumTrianglesRendered += nNumTriangles;
                    sObjStats.m_nNumInstances += sInstanceStats.m_nNumInstancesDrawn;
                }
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::RenderBillboards

CForestRI_TemplateList
inline st_bool CForestRI_t::RenderBillboards(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances) const
{
    ScopeTrace(ePass == RENDER_PASS_MAIN ? "CForestRI_t::RenderBillboards(Lit)" : "CForestRI_t::RenderBillboards(Depth-only)");

    st_bool bSuccess = true;

    // get data structure that contains the base trees with their associated billboard instancing structs
    //const typename CVisibleInstancesRI_t::TInstanceDataPtrArray& aPerBaseBillboardInstancingData = cVisibleInstances.GetPerBaseInstancingData( );
    const typename CVisibleInstancesRI_t::TInstanceDataPtrMap& mPerBaseBillboardInstancingDataMap = cVisibleInstances.GetPerBaseInstancingDataMap( );

    // run through each base tree, rendering its billboards
    for (typename CVisibleInstancesRI_t::TInstanceDataPtrMap::const_iterator i = mPerBaseBillboardInstancingDataMap.begin( ); i != mPerBaseBillboardInstancingDataMap.end( ); ++i)
    //for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aPerBaseBillboardInstancingData.size( )); ++nBaseTree)
    {
        // get base tree
        const CTreeRI_t* pBaseTree = (const CTreeRI_t*) i->first;
        assert(pBaseTree);

        // get instance manager
        const TInstancingMgrClass& tBillboardIM = i->second->m_tBillboardInstancingMgr;

        // if there's no billboards for this base true, just move to the next
        if (tBillboardIM.NumInstances( ) == 0)
            continue;
        assert(tBillboardIM.IsInitialized( ));

        // stats tracking
        CRenderStats::SObjectStats& sObjStats = m_cRenderStats.GetObjectStats("Billboards");

        const CRenderStateRI_t& c_sRenderState = pBaseTree->GetBillboardRenderState(ePass);
        if (c_sRenderState.BindMaterialWhole(ePass, m_sRenderInfo.m_bTexturingEnabled ? TEXTURE_BIND_ENABLED : TEXTURE_BIND_FALLBACK) &&
            pBaseTree->BindConstantBuffers( ))
        {
            // render billboard geometry for this base tree and retrieve stats
            SInstancedDrawStats sDrawStats;
            bSuccess &= tBillboardIM.RenderBillboards(sDrawStats);

            // stats tracking
            const SGeometry* c_pGeometry = pBaseTree->GetGeometry( );
            assert(c_pGeometry);
            sObjStats.m_nNumInstances += sDrawStats.m_nNumInstancesDrawn;
            sObjStats.m_nNumDrawCalls += sDrawStats.m_nNumDrawCalls;
            sObjStats.m_nNumTrianglesRendered += sDrawStats.m_nNumInstancesDrawn * c_pGeometry->m_sVertBBs.m_nNumCutoutIndices / 3 + (pBaseTree->GetGeometry( )->m_sHorzBB.m_bPresent ? 2 : 0);
        }
        else
            bSuccess = false;
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
//  CForestRI::WindAdvance

CForestRI_TemplateList
inline void CForestRI_t::WindAdvance(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs)
{
    // aBaseTrees includes grass models

    // CForest level advance (no gfx calls)
    CForest::WindAdvance(aBaseTrees, fWallTimeInSecs);

    // add gfx calls
    for (size_t i = 0;i < aBaseTrees.size( ); ++i)
        ((CTreeRI_t*) aBaseTrees[i])->UpdateWindConstantBuffer( );
}


////////////////////////////////////////////////////////////
//  CForestRI::GetFrameConstantBuffer

CForestRI_TemplateList
inline SFrameConstBuf_t& CForestRI_t::GetFrameConstantBuffer(void)
{
    return m_sFrameConstantBuffer;
}


////////////////////////////////////////////////////////////
//  CForestRI::UpdateFrameConstantBuffer
//
//  These shader parameters are all dependent on the current view, so expect
//  this function to be called multiple times in a multipass render.

CForestRI_TemplateList
inline st_bool CForestRI_t::UpdateFrameConstantBuffer(const CView& cView, st_int32 nWindowWidth, st_int32 nWindowHeight)
{
    bool bSuccess = true;

    // projections
    m_sFrameConstantBuffer.u_mModelViewProj3d = cView.GetCompositeNoTranslate( );
    m_sFrameConstantBuffer.u_mProjectionInverse3d = cView.GetProjectionInverse( );
    m_sFrameConstantBuffer.u_mCameraFacingMatrix = cView.GetCameraFacingMatrix( );

    // set 2d projection value
    Mat4x4 mProjection;
    mProjection.Ortho(0.0f, st_float32(nWindowWidth), 0.0f, st_float32(nWindowHeight), 0.1f, 2.0f);
    Mat4x4 mModelview;
    mModelview.Translate(0.0f, 0.0f, -1.0f);
    m_sFrameConstantBuffer.u_mModelViewProj2d = mModelview * mProjection;

    // other camera
    m_sFrameConstantBuffer.u_vCameraPosition = cView.GetCameraPos( );
    m_sFrameConstantBuffer.u_vCameraDirection = cView.GetCameraDir( );
    m_sFrameConstantBuffer.u_vLodRefPosition = cView.GetLodRefPoint( );

    m_sFrameConstantBuffer.u_vViewport = Vec2(st_float32(nWindowWidth), st_float32(nWindowHeight));
    m_sFrameConstantBuffer.u_vViewportInverse = Vec2(1.0f / m_sFrameConstantBuffer.u_vViewport.x, 1.0f / m_sFrameConstantBuffer.u_vViewport.y);
    m_sFrameConstantBuffer.u_fFarClip = cView.GetFarClip( );

    // misc
    m_sFrameConstantBuffer.u_fWallTime = CForest::WindGetGlobalTime( );

    // shadows
    if (m_sRenderInfo.m_bShadowsEnabled)
    {
        // convert ranges in world units to ranges in [0,1] range by dividing my far clip value;
        // unused maps must default to 1.0f
        for (st_int32 nMap = 0; nMap < m_sRenderInfo.m_nShadowsNumMaps; ++nMap)
            m_sFrameConstantBuffer.u_sShadows.m_vMapRanges[nMap] = (nMap < m_sRenderInfo.m_nShadowsNumMaps) ? m_sRenderInfo.m_afShadowMapRanges[nMap] / m_sRenderInfo.m_fFarClip : 1.0f;

        assert(m_sRenderInfo.m_nShadowsNumMaps > 0);
        const st_float32 c_fFurthestShadowPercent = m_sFrameConstantBuffer.u_sShadows.m_vMapRanges[m_sRenderInfo.m_nShadowsNumMaps - 1]; // shadow fully faded by this distance
        m_sFrameConstantBuffer.u_sShadows.m_fFadeStartPercent = m_sRenderInfo.m_fShadowFadePercent * c_fFurthestShadowPercent;
        m_sFrameConstantBuffer.u_sShadows.m_fFadeInverseDistance = 1.0f / (c_fFurthestShadowPercent - m_sFrameConstantBuffer.u_sShadows.m_fFadeStartPercent);
        m_sFrameConstantBuffer.u_sShadows.m_fTerrainAmbientOcclusion = 1.0f / (m_sFrameConstantBuffer.u_sShadows.m_fFadeStartPercent - c_fFurthestShadowPercent);

        const st_float32 c_fOffset = 1.0f / m_sRenderInfo.m_nShadowsResolution;
        m_sFrameConstantBuffer.u_sShadows.m_avSmoothingTable[0].Set(0.0f, c_fOffset, 0.0f, 0.0f);
        m_sFrameConstantBuffer.u_sShadows.m_avSmoothingTable[1].Set(-c_fOffset, -c_fOffset, 0.0f, 0.0f);
        m_sFrameConstantBuffer.u_sShadows.m_avSmoothingTable[2].Set(-c_fOffset, 0.0f, 0.0f, 0.0f);

        m_sFrameConstantBuffer.u_sShadows.m_vTexelOffset.Set(0.5f / m_sRenderInfo.m_nShadowsResolution, 0.5f / m_sRenderInfo.m_nShadowsResolution);
    }

    bSuccess &= m_sFrameConstantBuffer.Update( );

    return bSuccess;
}


////////////////////////////////////////////////////////////
//  CForestRI::UpdateFogAndSkyConstantBuffer

CForestRI_TemplateList
inline st_bool CForestRI_t::UpdateFogAndSkyConstantBuffer(void)
{
    m_sFogAndSkyConstantBuffer.u_vFogColor = m_sRenderInfo.m_vFogColor;

    // assuming linear fog
    m_sFogAndSkyConstantBuffer.u_fFogEndDist = m_sRenderInfo.m_fFogEndDistance;
    m_sFogAndSkyConstantBuffer.u_fFogSpan = (m_sRenderInfo.m_fFogEndDistance - m_sRenderInfo.m_fFogStartDistance);
    m_sFogAndSkyConstantBuffer.u_vSkyColor = m_sRenderInfo.m_vSkyColor;
    m_sFogAndSkyConstantBuffer.u_vSunColor = m_sRenderInfo.m_vSunColor;
    m_sFogAndSkyConstantBuffer.u_fSunSize = m_sRenderInfo.m_fSunSize;
    m_sFogAndSkyConstantBuffer.u_fSunSpreadExponent = m_sRenderInfo.m_fSunSpreadExponent;

    return (m_sFogAndSkyConstantBuffer.IsValid( ) ? m_sFogAndSkyConstantBuffer.Update( ) : true);
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::GetRenderStats

CForestRI_TemplateList
inline CRenderStats& CForestRI_t::GetRenderStats(void)
{
    return m_cRenderStats;
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::SDrawCallData::operator<

CForestRI_TemplateList
inline st_bool CForestRI_t::SDrawCallData::operator<(const SDrawCallData& sRight) const
{
    // sort for branch type, then base tree, then lowest LOD
    if (GetHashKey( ) == sRight.GetHashKey( ))
        return (m_nLod < sRight.m_nLod);
    else
        return (GetHashKey( ) < sRight.GetHashKey( ));
}


///////////////////////////////////////////////////////////////////////
//  CForestRI::SDrawCallData::GetHashKey

CForestRI_TemplateList
inline st_int64 CForestRI_t::SDrawCallData::GetHashKey(void) const
{
    assert(m_pRenderState);

    return m_pRenderState->GetHashKey( );
}

