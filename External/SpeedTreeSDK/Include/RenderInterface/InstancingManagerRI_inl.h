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
//  SInstancedDrawStats::SInstancedDrawStats

inline SInstancedDrawStats::SInstancedDrawStats( ) :
    m_nNumInstancesDrawn(0),
    m_nNumDrawCalls(0),
    m_nBatchSize(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::CInstancingMgrRI

CInstancingMgrRI_TemplateList
inline CInstancingMgrRI_t::CInstancingMgrRI( ) :
    m_nActiveMgrIndex(0),
    m_bInitialized(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::~CInstancingMgrRI

CInstancingMgrRI_TemplateList
inline CInstancingMgrRI_t::~CInstancingMgrRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Init3dTrees

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Init3dTrees(st_int32 nNumLods, const CArray<TGeometryBufferClass>& aGeometryBuffers)
{
    if (!aGeometryBuffers.empty( ))
    {
        m_bInitialized = true;
        for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
            m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_3D_TREES, nNumLods, &aGeometryBuffers[0], st_int32(aGeometryBuffers.size( )));

        return m_bInitialized;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::InitGrass

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::InitGrass(const CArray<TGeometryBufferClass>& aGeometryBuffers)
{
    if (!aGeometryBuffers.empty( ))
    {
        m_bInitialized = true;
        for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
            m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_GRASS, 1, &aGeometryBuffers[0], st_int32(aGeometryBuffers.size( )));

        return m_bInitialized;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::InitBillboards

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::InitBillboards(const TGeometryBufferClass* pGeometryBuffer)
{
    if (pGeometryBuffer)
    {
        m_bInitialized = true;
        for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
            m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_BILLBOARDS, 1, pGeometryBuffer, 1);

        return m_bInitialized;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::ReleaseGfxResources

CInstancingMgrRI_TemplateList
inline void CInstancingMgrRI_t::ReleaseGfxResources(void)
{
    for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
        m_atInstanceMgrPolicies[i].ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  Structure SBufferDataByLod

struct SBufferDataByLod
{
                    SBufferDataByLod( ) :
                        m_pGpuInstanceBufferBase(NULL),
                        m_pGpuInstanceBufferPtr(NULL)
                    {
                    }

    SInstance*      m_pGpuInstanceBufferBase;
    SInstance*      m_pGpuInstanceBufferPtr;
};


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Update3dTreeInstanceBuffers

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Update3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods)
{
    st_bool bSuccess = true;

    if (!aInstanceLods.empty( ) && nNumLods > 0)
    {
        AdvanceMgrIndex( );

        // map a GPU buffer for each LOD level
        CArray<SBufferDataByLod> aGpuBuffersByLod(nNumLods);

        for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
            aGpuBuffersByLod[nLod].m_pGpuInstanceBufferBase = aGpuBuffersByLod[nLod].m_pGpuInstanceBufferPtr = (SInstance*) m_atInstanceMgrPolicies[m_nActiveMgrIndex].LockInstanceBufferForWrite(nLod, st_int32(aInstanceLods.size( )));

        for (st_int32 i = 0; i < st_int32(aInstanceLods.size( )); ++i)
        {
            const S3dTreeInstanceLod& sInstanceLod = aInstanceLods[i];
            const SInstance* pInstance = sInstanceLod.m_pInstance;

            assert(sInstanceLod.m_nLodLevel > -1 && sInstanceLod.m_nLodLevel < nNumLods);

            SInstance* pDst = aGpuBuffersByLod[sInstanceLod.m_nLodLevel].m_pGpuInstanceBufferPtr;

            // first float4
            pDst->m_vPos = pInstance->m_vPos;
            pDst->m_fScalar = pInstance->m_fScalar;

            // second float4
            pDst->m_vUp = pInstance->m_vUp;
            pDst->m_fLodTransition = sInstanceLod.m_fLodTransition;

            // third float4
            pDst->m_vRight = pInstance->m_vRight;
            pDst->m_fLodValue = sInstanceLod.m_fLod;

            ++aGpuBuffersByLod[sInstanceLod.m_nLodLevel].m_pGpuInstanceBufferPtr;
        }

        for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
            m_atInstanceMgrPolicies[m_nActiveMgrIndex].UnlockInstanceBufferFromWrite(nLod, st_int32(aGpuBuffersByLod[nLod].m_pGpuInstanceBufferPtr - aGpuBuffersByLod[nLod].m_pGpuInstanceBufferBase));
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::UpdateGrassInstanceBuffers

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::UpdateGrassInstanceBuffers(const TRowColCellPtrMap& mCells)
{
    st_bool bSuccess = true;

    const st_int32 c_nGrassLodLevel = 0;

    AdvanceMgrIndex( );

    // do a quick scan to figure out how large of a buffer we need
    st_int32 nNumGrassInstances = 0;
    for (TRowColCellPtrMap::const_iterator i = mCells.begin( ); i != mCells.end( ); ++i)
    {
        assert(i->second);
        nNumGrassInstances += st_int32(i->second->GetGrassInstances( ).size( ));
    }

    if (nNumGrassInstances > 0)
    {
        // get a pointer to the GPU instance buffer
        SInstance* pInstances = (SInstance*) m_atInstanceMgrPolicies[m_nActiveMgrIndex].LockInstanceBufferForWrite(c_nGrassLodLevel, nNumGrassInstances);
        assert(pInstances);
        SInstance* pIntermediateBufPtr = pInstances;

        // rebuild a single inst-buffer for all grass instances of this type; some of these cells have been around for a while,
        // some have just been added, but they all contribute to the single inst-buffer
        for (TRowColCellPtrMap::const_iterator i = mCells.begin( ); i != mCells.end( ); ++i)
        {
            const TGrassInstArray& aInstances = i->second->GetGrassInstances( );
            if (!aInstances.empty( ))
            {
                memcpy(pIntermediateBufPtr, &aInstances[0], aInstances.size( ) * sizeof(SInstance));
                pIntermediateBufPtr += aInstances.size( );
            }
        }

        // unlock the GPU instance buffer and 
        bSuccess = m_atInstanceMgrPolicies[m_nActiveMgrIndex].UnlockInstanceBufferFromWrite(c_nGrassLodLevel, nNumGrassInstances);
    }
    else
    {
        // clear the instance buffer -- grass is not visible for this cell group
        if (m_atInstanceMgrPolicies[m_nActiveMgrIndex].LockInstanceBufferForWrite(c_nGrassLodLevel, 0))
            (void) m_atInstanceMgrPolicies[m_nActiveMgrIndex].UnlockInstanceBufferFromWrite(c_nGrassLodLevel, 0);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::CopyBillboardInstancesToGpu

CInstancingMgrRI_TemplateList
inline st_int32 CInstancingMgrRI_t::CopyBillboardInstancesToGpu(const CTree* pBaseTree, const TRowColCellPtrMap& mVisibleCells)
{
    // how many instances are we talking about?
    st_int32 nNumBillboards = 0;
    for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
    {
        if (i->second->GetLodDistanceSquared( ) < pBaseTree->GetLodProfileSquared( ).m_fBillboardCullDistance)
        {
            CMap<const CTree*, CArray<SInstance> >::const_iterator iFind = i->second->m_mBaseTreesToBillboardVboStreamsMap.find(pBaseTree);

            if (iFind != i->second->m_mBaseTreesToBillboardVboStreamsMap.end( ))
                nNumBillboards += st_int32(iFind->second.size( ));
        }
    }

    if (nNumBillboards > 0)
    {
        AdvanceMgrIndex( );

        SInstance* pGpuBuffer = (SInstance*) m_atInstanceMgrPolicies[m_nActiveMgrIndex].LockInstanceBufferForWrite(0, nNumBillboards);
        {
            for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
            {
                if (i->second->GetLodDistanceSquared( ) < pBaseTree->GetLodProfileSquared( ).m_fBillboardCullDistance)
                {
                    CMap<const CTree*, CArray<SInstance> >::const_iterator iFind = i->second->m_mBaseTreesToBillboardVboStreamsMap.find(pBaseTree);
                    if (iFind != i->second->m_mBaseTreesToBillboardVboStreamsMap.end( ))
                    {
                        const size_t c_nNumBillboardsInCell = iFind->second.size( );

                        memcpy(pGpuBuffer, &(iFind->second[0]), sizeof(SInstance) * c_nNumBillboardsInCell);
                        pGpuBuffer += c_nNumBillboardsInCell;
                    }
                }
            }
        }
        m_atInstanceMgrPolicies[m_nActiveMgrIndex].UnlockInstanceBufferFromWrite(0, nNumBillboards);
    }

    return nNumBillboards;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Render3dTrees

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Render3dTrees(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const
{
    return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(nGeometryBufferIndex, nLod, sStats);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::RenderGrass

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::RenderGrass(st_int32 nGeometryBufferIndex, SInstancedDrawStats& sStats) const
{
    return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(nGeometryBufferIndex, 0, sStats);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::RenderBillboards

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::RenderBillboards(SInstancedDrawStats& sStats) const
{
    return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(0, 0, sStats);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::IsInitialized

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::IsInitialized(void) const
{
    return m_bInitialized;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::NumInstances

CInstancingMgrRI_TemplateList
inline st_int32 CInstancingMgrRI_t::NumInstances(st_int32 nLod) const
{
    return IsInitialized( ) ? m_atInstanceMgrPolicies[m_nActiveMgrIndex].NumInstances(nLod) : 0;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::AdvanceMgrIndex

CInstancingMgrRI_TemplateList
inline void CInstancingMgrRI_t::AdvanceMgrIndex(void)
{
    assert(c_nNumInstBuffers > 0);

    m_nActiveMgrIndex = (m_nActiveMgrIndex + 1) % c_nNumInstBuffers;
}
