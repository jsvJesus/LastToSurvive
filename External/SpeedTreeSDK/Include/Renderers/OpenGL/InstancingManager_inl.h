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
//  CInstancingMgrOpenGL::CInstancingMgrOpenGL

inline CInstancingMgrOpenGL::CInstancingMgrOpenGL( ) :
    m_pObjectBuffers(NULL),
    m_siSizeOfPerInstanceData(0),
    m_bMeshInstancingSupported(false)
{
    m_asInstances.SetHeapDescription("CInstancingMgrOpenGL::m_asInstances");
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::~CInstancingMgrOpenGL

inline CInstancingMgrOpenGL::~CInstancingMgrOpenGL( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::SInstancesPerLod::SInstancesPerLod

inline CInstancingMgrOpenGL::SInstancesPerLod::SInstancesPerLod( ) :
    m_nNumInstances(0),
    m_uiInstanceBuffer(0),
    m_siInstanceBufferSize(0),
    m_bBufferLocked(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::Init

inline st_bool CInstancingMgrOpenGL::Init(SVertexDecl::EInstanceType eInstanceType, st_int32 nNumLods, const CGeometryBuffer* pGeometryBuffers, st_int32 nNumGeometryBuffers)
{
    st_bool bSuccess = false;

    m_bMeshInstancingSupported = CGraphicsCapsOpenGL::IsInstancingSupported( );

    if (eInstanceType != SVertexDecl::INSTANCES_NONE && pGeometryBuffers && nNumGeometryBuffers > 0)
    {
        m_siSizeOfPerInstanceData = sizeof(SInstance);

        // retain geometry buffers ptr, we'll have to call Bind() on them in Render()
        m_pObjectBuffers = pGeometryBuffers;

        // set size of per-instance array based on number of LOD levels
        m_asInstances.resize(nNumLods);
        for (st_int32 i = 0; i < nNumLods; ++i)
            glGenBuffers(1, &m_asInstances[i].m_uiInstanceBuffer);

        // init VAO array; create upon first draw
        m_aVAOs.resize(nNumGeometryBuffers);
        memset(&m_aVAOs[0], 0, sizeof(GLuint) * nNumGeometryBuffers);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::ReleaseGfxResources

inline void CInstancingMgrOpenGL::ReleaseGfxResources(void)
{
    // intentionally empty
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::Update

inline st_bool CInstancingMgrOpenGL::Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances)
{
    assert(nLod > -1);
    assert(nLod < st_int32(m_asInstances.size( )));
    assert(m_siSizeOfPerInstanceData > 0);

    st_bool bSuccess = false;

    m_asInstances[nLod].m_nNumInstances = nNumInstances;
    if (m_asInstances[nLod].m_uiInstanceBuffer != 0)
    {
        if (pInstanceData && nNumInstances > 0)
        {
            const size_t siSizeToCopy = nNumInstances * m_siSizeOfPerInstanceData;

            glBindBuffer(GL_ARRAY_BUFFER, m_asInstances[nLod].m_uiInstanceBuffer);
            {
                glBufferData(GL_ARRAY_BUFFER, siSizeToCopy, pInstanceData, GL_DYNAMIC_DRAW);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::Render

inline st_bool CInstancingMgrOpenGL::Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const
{
    assert(m_pObjectBuffers);
    assert(m_siSizeOfPerInstanceData > 0);
    assert(nLod < st_int32(m_asInstances.size( )));

    st_bool bSuccess = false;

    // set up a few aliases
    const SInstancesPerLod& sInstanceData = m_asInstances[nLod];
    const CGeometryBuffer* pObjectBuffer = m_pObjectBuffers + nGeometryBufferIndex;
    assert(pObjectBuffer);

    // check for early exit conditions
    if (pObjectBuffer->NumIndices( ) == 0 || sInstanceData.m_nNumInstances == 0)
        return true;

    // create VAO if it hasn't already been; each VAO holds a unique combination of the instance buffer for LOD level nLod and the
    // geometry buffer being rendered (e.g. the branches for LOD 1 of a particular tree). nGeometryBufferIndex holds the correct index
    // value of LOD level and geometry buffer.
    if (m_aVAOs[nGeometryBufferIndex] == 0)
    {
        assert(sInstanceData.m_uiInstanceBuffer != 0);
        m_aVAOs[nGeometryBufferIndex] = pObjectBuffer->m_tGeometryBufferPolicy.CreateInstancingVAO(pObjectBuffer, sInstanceData.m_uiInstanceBuffer);
    }

    // query currently bound VAO
    GLint iCurrentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &iCurrentVAO);

    // bind and render instanced geometry
    assert(m_aVAOs[nGeometryBufferIndex] != 0);
    glBindVertexArray(m_aVAOs[nGeometryBufferIndex]);
    {
        glDrawElementsInstanced(GL_TRIANGLES, 
                                pObjectBuffer->NumIndices( ), 
                                pObjectBuffer->IndexSize( ) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                0,
                                sInstanceData.m_nNumInstances);
    }

    // restore previously-bound VAO
    glBindVertexArray(iCurrentVAO);

    // update stats
    sStats.m_nNumDrawCalls++;
    sStats.m_nNumInstancesDrawn += sInstanceData.m_nNumInstances;

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::LockInstanceBufferForWrite

inline void* CInstancingMgrOpenGL::LockInstanceBufferForWrite(st_int32 nLod, st_int32 nMaxNumInstances)
{
    void* pBuffer = NULL;

    if (nLod < st_int32(m_asInstances.size( )))
    {
        SInstancesPerLod& sInstanceData = m_asInstances[nLod];

        glBindBuffer(GL_ARRAY_BUFFER, sInstanceData.m_uiInstanceBuffer);

        // track the size of the buffer
        size_t siRequestedBufferSize = nMaxNumInstances * m_siSizeOfPerInstanceData;
        sInstanceData.m_siInstanceBufferSize = st_max(sInstanceData.m_siInstanceBufferSize, siRequestedBufferSize);

        // clear contents of buffer before mapping for speed
        glBufferData(GL_ARRAY_BUFFER, sInstanceData.m_siInstanceBufferSize, NULL, GL_DYNAMIC_DRAW);

        // finally, map the buffer
        pBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (pBuffer)
            sInstanceData.m_bBufferLocked = true;
    }

    return pBuffer;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::UnlockInstanceBufferFromWrite

inline st_bool CInstancingMgrOpenGL::UnlockInstanceBufferFromWrite(st_int32 nLod, st_int32 nActualNumInstances)
{
    st_bool bSuccess = false;

    if (nLod < st_int32(m_asInstances.size( )) && m_asInstances[nLod].m_bBufferLocked)
    {
        SInstancesPerLod& sInstanceData = m_asInstances[nLod];

        glBindBuffer(GL_ARRAY_BUFFER, sInstanceData.m_uiInstanceBuffer);
        bSuccess = (glUnmapBuffer(GL_ARRAY_BUFFER) == GL_TRUE);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        sInstanceData.m_bBufferLocked = false;
        sInstanceData.m_nNumInstances = nActualNumInstances;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrOpenGL::NumInstances

inline st_int32 CInstancingMgrOpenGL::NumInstances(st_int32 nLod) const
{
    return m_asInstances[nLod].m_nNumInstances;
}



