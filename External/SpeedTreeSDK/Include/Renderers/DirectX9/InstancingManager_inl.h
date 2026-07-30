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
//  Constants

const st_int32 c_nInitialInstanceBatchSize = 1000;


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::CInstancingMgrDirectX9

inline CInstancingMgrDirectX9::CInstancingMgrDirectX9( ) :
    m_pObjectBuffers(NULL),
    m_siSizeOfPerInstanceData(0)
{
    m_aInstances.SetHeapDescription("CInstancingMgrDirectX9::m_aInstances");
    m_aCompositeVertexFormats.SetHeapDescription("CInstancingMgrDirectX9::m_aCompositeVertexFormats");
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::~CInstancingMgrDirectX9

inline CInstancingMgrDirectX9::~CInstancingMgrDirectX9( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::SInstancesPerLod::SInstancesPerLod

inline CInstancingMgrDirectX9::SInstancesPerLod::SInstancesPerLod( ) :
    m_nNumInstances(0),
    m_bBufferLocked(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::Init

inline st_bool CInstancingMgrDirectX9::Init(SVertexDecl::EInstanceType eInstanceType, 
                                            st_int32 nNumLods, 
                                            const CGeometryBuffer* pGeometryBuffers, 
                                            st_int32 nNumGeometryBuffers)
{
    st_bool bSuccess = false;

    if (eInstanceType != SVertexDecl::INSTANCES_NONE && pGeometryBuffers && nNumGeometryBuffers > 0)
    {
        bSuccess = true;

        m_siSizeOfPerInstanceData = sizeof(SInstance);

        SVertexDecl sInstanceDecl;
        sInstanceDecl.Set(c_asInstanceStreamDesc);

        // retain copies of some parameters
        m_pObjectBuffers = pGeometryBuffers;

        // set size of per-instance array based on number of LOD levels
        m_aInstances.resize(nNumLods);

        // initialize instance buffers; one per LOD
        for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
        {
            CGeometryBuffer& cInstanceBuffer = m_aInstances[nLod].m_cInstanceBuffer;
            if (cInstanceBuffer.SetVertexDecl(sInstanceDecl, NULL))
            {
                bSuccess = cInstanceBuffer.CreateUninitializedVertexBuffer(c_nInitialInstanceBatchSize);
            }
            else
            {
                CCore::SetError("CInstancingMgrDirectX9::Init, SetVertexDecl() failed");
                bSuccess = false;
            }
        }

        // setup composite vertex format descriptions (via empty CGeometryBuffer objects); one for each element in aGeometryBuffers
        if (bSuccess)
        {
            // set up the right quantity
            m_aCompositeVertexFormats.resize(nNumGeometryBuffers);

            for (st_int32 nBuffer = 0; nBuffer < nNumGeometryBuffers; ++nBuffer)
            {
                // not every geometry buffer will necessarily be in use
                if (pGeometryBuffers[nBuffer].VertexSize( ) > 0)
                {
                    SVertexDecl sMergedDecl;
                    if (SVertexDecl::MergeObjectAndInstanceVertexDecls(sMergedDecl, pGeometryBuffers[nBuffer].GetVertexDecl( ), eInstanceType))
                    {
                        bSuccess &= m_aCompositeVertexFormats[nBuffer].SetVertexDecl(sMergedDecl, NULL);
                    }
                }
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::ReleaseGfxResources

inline void CInstancingMgrDirectX9::ReleaseGfxResources(void)
{
    for (size_t i = 0; i < m_aInstances.size( ); ++i)
        m_aInstances[i].m_cInstanceBuffer.ReleaseGfxResources( );

    for (size_t i = 0; i < m_aCompositeVertexFormats.size( ); ++i)
        m_aCompositeVertexFormats[i].ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::Update

inline st_bool CInstancingMgrDirectX9::Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances)
{
    assert(nLod > -1);
    assert(nLod < st_int32(m_aInstances.size( )));
    assert(m_siSizeOfPerInstanceData > 0);

    st_bool bSuccess = true;

    m_aInstances[nLod].m_nNumInstances = nNumInstances;
    if (pInstanceData && nNumInstances > 0)
    {
        bSuccess = m_aInstances[nLod].m_cInstanceBuffer.OverwriteVertices(pInstanceData, nNumInstances, 0, c_nInstanceVertexStream);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::Render

inline st_bool CInstancingMgrDirectX9::Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const
{
    assert(m_pObjectBuffers);
    assert(m_siSizeOfPerInstanceData > 0);
    assert(nLod < st_int32(m_aInstances.size( )));

    st_bool bSuccess = false;

    // set up a few aliases
    const SInstancesPerLod& sInstanceData = m_aInstances[nLod];
    const CGeometryBuffer* pObjectBuffer = m_pObjectBuffers + nGeometryBufferIndex;
    assert(pObjectBuffer);

    // check for early exit conditions
    if (pObjectBuffer->NumIndices( ) == 0 || sInstanceData.m_nNumInstances == 0)
        return true;

    const CGeometryBuffer& cInstanceBuffer = sInstanceData.m_cInstanceBuffer;
    const CGeometryBuffer* pCompositeFormat = &m_aCompositeVertexFormats[nGeometryBufferIndex];

    if (pObjectBuffer->BindIndexBuffer( ))
    {
        if (cInstanceBuffer.BindVertexBuffer(c_nInstanceVertexStream) && 
            cInstanceBuffer.m_tGeometryBufferPolicy.SetStreamFrequency(c_nInstanceVertexStream, D3DSTREAMSOURCE_INSTANCEDATA | 1ul))
        {
            assert(sInstanceData.m_nNumInstances * cInstanceBuffer.VertexSize( ) <= cInstanceBuffer.m_tGeometryBufferPolicy.m_uiCurrentVertexBufferSize);

            if (pObjectBuffer->BindVertexBuffer(c_nObjectVertexStream) && 
                pObjectBuffer->m_tGeometryBufferPolicy.SetStreamFrequency(c_nObjectVertexStream, D3DSTREAMSOURCE_INDEXEDDATA | (unsigned long) sInstanceData.m_nNumInstances))
            {
                if (pCompositeFormat->EnableFormat( ))
                {
                    bSuccess = pObjectBuffer->RenderIndexed(PRIMITIVE_TRIANGLES, 0, pObjectBuffer->NumIndices( ), 0, pObjectBuffer->NumVertices( ));

                    // update stats
                    sStats.m_nNumDrawCalls++;
                    sStats.m_nNumInstancesDrawn += sInstanceData.m_nNumInstances;
                    sStats.m_nBatchSize = sInstanceData.m_nNumInstances;

                    bSuccess &= pCompositeFormat->DisableFormat( );
                }

                bSuccess &= pObjectBuffer->UnBindVertexBuffer(c_nObjectVertexStream);
                bSuccess &= pObjectBuffer->m_tGeometryBufferPolicy.SetStreamFrequency(c_nObjectVertexStream, 1);
            }

            bSuccess &= cInstanceBuffer.UnBindVertexBuffer(c_nInstanceVertexStream);
            bSuccess &= cInstanceBuffer.m_tGeometryBufferPolicy.SetStreamFrequency(c_nInstanceVertexStream, 1);
        }

        bSuccess &= pObjectBuffer->UnBindIndexBuffer( );
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::LockInstanceBufferForWrite

inline void* CInstancingMgrDirectX9::LockInstanceBufferForWrite(st_int32 nLod, st_int32 nMaxNumInstances)
{
    void* pBuffer = NULL;

    if (nLod < st_int32(m_aInstances.size( )))
    {
        pBuffer = m_aInstances[nLod].m_cInstanceBuffer.LockForWrite(st_uint32(nMaxNumInstances * m_siSizeOfPerInstanceData));
        m_aInstances[nLod].m_bBufferLocked = true;
    }

    return pBuffer;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::UnlockInstanceBufferFromWrite

inline st_bool CInstancingMgrDirectX9::UnlockInstanceBufferFromWrite(st_int32 nLod, st_int32 nActualNumInstances)
{
    st_bool bSuccess = false;

    if (nLod < st_int32(m_aInstances.size( )) && m_aInstances[nLod].m_bBufferLocked)
    {
        bSuccess = m_aInstances[nLod].m_cInstanceBuffer.UnlockFromWrite( );
        m_aInstances[nLod].m_nNumInstances = nActualNumInstances;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX9::NumInstances

inline st_int32 CInstancingMgrDirectX9::NumInstances(st_int32 nLod) const
{
    return m_aInstances[nLod].m_nNumInstances;
}
