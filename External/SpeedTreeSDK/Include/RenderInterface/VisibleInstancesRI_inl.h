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
//  SForestCullResultsRI_t::SForestInstancingData::SForestInstancingData

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::SForestInstancingData::SForestInstancingData( ) :
    m_pBaseTree(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::CVisibleInstancesRI

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::CVisibleInstancesRI(EPopulationType ePopulationType, st_bool bTrackNearestInsts) :
    CVisibleInstances(ePopulationType, bTrackNearestInsts),
    m_mPerBaseInstancingDataMap(30)
{
    m_mPerBaseInstancingDataMap.SetHeapDescription("CVisibleInstancesRI::m_mPerBaseInstancingDataMap");
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::~CVisibleInstancesRI

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::~CVisibleInstancesRI( )
{
    for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
        st_delete<SForestInstancingData>(i->second);
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::SetHeapReserves

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
    // base-class specific
    {
        ScopeTrace("CVisibleInstances::SetHeapReserves");
        CVisibleInstances::SetHeapReserves(sHeapReserves);
    }
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::InitGfx

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::InitGfx(const CArray<CTreeRI_t*>& aBaseTrees)
{
    st_bool bSuccess = true;

    for (size_t i = 0; i < aBaseTrees.size( ); ++i)
    {
        CTreeRI_t* pBaseTree = aBaseTrees[i];

        // find the VB associate with this base tree
        typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
        assert(pInstancingData);

        // initialize instance manager if not already
        if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
        {
            // get base tree geometry
            assert(pBaseTree->GetGeometry( ));
            const st_int32 c_nNumLods = pBaseTree->GetGeometry( )->m_nNumLods;

            bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Init3dTrees(c_nNumLods, pBaseTree->GetGeometryBuffers( ));
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::InitGfx

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::Clear(void)
{
    m_mPerBaseInstancingDataMap.clear( );
    CVisibleInstances::Clear( );
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::ReleaseGfxResources

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::ReleaseGfxResources(void)
{
    for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
    {
        assert(i->second);
        i->second->m_t3dTreeInstancingMgr.ReleaseGfxResources( );
        i->second->m_tBillboardInstancingMgr.ReleaseGfxResources( );
    }
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::Cull3dTreesAndUpdateInstanceBuffers

CVisibleInstancesRI_TemplateList
inline st_int32 CVisibleInstancesRI_t::Cull3dTreesAndUpdateInstanceBuffers(const CView& cView)
{
    st_bool bSuccess = true;
    st_int32 nNum3dTrees = 0;

    {
        ScopeTrace("Cull3dTreesAndComputeLod");
        Cull3dTreesAndComputeLod(cView);
    }

    ScopeTrace("InstMgr::Update3dTreeInstanceBuffers");
    const TDetailedCullDataArray& aPerBase3dInstances = Get3dInstanceLods( );
    for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
    {
        // get the instances for this base tree index
        const T3dTreeInstanceLodArray& aInstanceLodsForBase = aPerBase3dInstances[nBase].m_a3dInstanceLods;
        if (!aInstanceLodsForBase.empty( ))
        {
            nNum3dTrees += st_int32(aInstanceLodsForBase.size( ));

            // access base tree as render interface type
            CTreeRI_t* pBaseTree = (CTreeRI_t*) aPerBase3dInstances[nBase].m_pBaseTree;
            assert(pBaseTree);

            // get base tree geometry
            assert(pBaseTree->GetGeometry( ));
            const st_int32 c_nNumLods = pBaseTree->GetGeometry( )->m_nNumLods;

            // find the VB associate with this base tree
            typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
            assert(pInstancingData);

            // initialize instance manager if not already
            if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
                bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Init3dTrees(c_nNumLods, pBaseTree->GetGeometryBuffers( ));

            bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Update3dTreeInstanceBuffers(c_nNumLods, aInstanceLodsForBase);
        }
    }

    return (bSuccess ? nNum3dTrees: -1);
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::UpdateGrassInstanceBuffers

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::UpdateGrassInstanceBuffers(const CTreeRI_t* pBaseGrass)
{
    assert(pBaseGrass);

    st_bool bSuccess = true;

    // find the instance data associated with this base grass object
    typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseGrass);
    assert(pInstancingData);

    // initialize the instance manager if not already
    if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
    {
        assert(!pBaseGrass->GetGeometryBuffers( ).empty( ));
        bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.InitGrass(pBaseGrass->GetGeometryBuffers( ));
    }

    bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.UpdateGrassInstanceBuffers(VisibleCells( ));

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::CopyBillboardInstancesToGpu

CVisibleInstancesRI_TemplateList
inline st_int32 CVisibleInstancesRI_t::CopyBillboardInstancesToGpu(const CTreeRI_t* pBaseTree)
{
    assert(pBaseTree);

    st_bool bSuccess = true;
    st_int32 nNumBBsCopied = -1; // error condition by default

    typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = NULL;

    // find the instance data associated with this base tree object
    pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
    assert(pInstancingData);

    // initialize the instance manager if not already (each base tree has its own billboard instance manager)
    if (!pInstancingData->m_tBillboardInstancingMgr.IsInitialized( ))
        bSuccess &= pInstancingData->m_tBillboardInstancingMgr.InitBillboards(&pBaseTree->GetBillboardGeometryBuffer( ));

    // perform copy via the instancing manager
    if (bSuccess)
        nNumBBsCopied = pInstancingData->m_tBillboardInstancingMgr.CopyBillboardInstancesToGpu(pBaseTree, VisibleCells( ));

    return nNumBBsCopied;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::NotifyOfPopulationChange

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::NotifyOfPopulationChange(void)
{
    // delete per-base-tree instance data
    for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
    {
        i->second->m_t3dTreeInstancingMgr.ReleaseGfxResources( );
        i->second->m_tBillboardInstancingMgr.ReleaseGfxResources( );
        st_delete<SForestInstancingData>(i->second);
    }
    m_mPerBaseInstancingDataMap.clear( );

    CVisibleInstances::NotifyOfPopulationChange( );
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::FindInstancingDataByBaseTree

CVisibleInstancesRI_TemplateList
inline const typename CVisibleInstancesRI_t::SForestInstancingData* CVisibleInstancesRI_t::FindInstancingDataByBaseTree(const CTree* pTree) const
{
    const SForestInstancingData* pData = NULL;

    if (pTree)
    {
        typename TInstanceDataPtrMap::const_iterator iFind = m_mPerBaseInstancingDataMap.find(pTree);
        if (iFind != m_mPerBaseInstancingDataMap.end( ))
            pData = iFind->second;
    }

    return pData;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::FindOrAddInstancingDataByBaseTree

CVisibleInstancesRI_TemplateList
inline typename CVisibleInstancesRI_t::SForestInstancingData* CVisibleInstancesRI_t::FindOrAddInstancingDataByBaseTree(const CTree* pTree)
{
    SForestInstancingData* pData = NULL;

    if (pTree)
    {
        typename TInstanceDataPtrMap::const_iterator iFind = m_mPerBaseInstancingDataMap.find(pTree);
        if (iFind != m_mPerBaseInstancingDataMap.end( ))
            pData = iFind->second;
        else
        {
            pData = st_new(SForestInstancingData, "SForestInstancingData");
            m_mPerBaseInstancingDataMap[pTree] = pData;

            pData->m_pBaseTree = pTree;
        }
    }

    return pData;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::GetPerBaseInstancingDataMap

CVisibleInstancesRI_TemplateList
inline const typename CVisibleInstancesRI_t::TInstanceDataPtrMap& CVisibleInstancesRI_t::GetPerBaseInstancingDataMap(void) const
{
    return m_mPerBaseInstancingDataMap;
}



