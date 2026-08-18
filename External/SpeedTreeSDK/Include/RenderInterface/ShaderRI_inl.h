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

//#define SHADER_CACHE_USES_FULL_PATHNAMES


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::CShaderTechniqueRI

CShaderTechniqueRI_TemplateList
inline CShaderTechniqueRI_t::CShaderTechniqueRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::~CShaderTechniqueRI

CShaderTechniqueRI_TemplateList
inline CShaderTechniqueRI_t::~CShaderTechniqueRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::Bind

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::Bind(void) const
{
    st_bool bSuccess = false;

    #if !defined(NDEBUG) && !defined(ST_OPENGL)
        if (IsValid( ))
        {
    #endif

            bSuccess = m_tShaderTechniquePolicy.Bind(m_tVertexShader, m_tPixelShader);

    #if !defined(NDEBUG) && !defined(ST_OPENGL)
        }
    #endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::UnBind

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::UnBind(void)
{
    TShaderTechniquePolicy::UnBind( );

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::IsValid

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::IsValid(void) const
{
    return m_tVertexShader.IsValid( ) &&
           m_tPixelShader.IsValid( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::CShaderConstantBufferRI

CShaderConstantBufferRI_TemplateList
inline CShaderConstantBufferRI_t::CShaderConstantBufferRI( ) :
    m_bIsValid(false)
{
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::~CShaderConstantBufferRI

CShaderConstantBufferRI_TemplateList
inline CShaderConstantBufferRI_t::~CShaderConstantBufferRI( )
{
    m_bIsValid = false;
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Init

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Init(size_t siSizeOfDerivedClass, st_int32 nRegister)
{
    m_bIsValid = m_tShaderConstantBufferPolicy.Init(siSizeOfDerivedClass, nRegister);

    return m_bIsValid;
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::ReleaseGfxResources

CShaderConstantBufferRI_TemplateList
inline void CShaderConstantBufferRI_t::ReleaseGfxResources(void)
{
    m_tShaderConstantBufferPolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::IsValid

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::IsValid(void) const
{
    return m_bIsValid;
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Update

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Update(void) const
{
    return m_tShaderConstantBufferPolicy.Update( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Bind

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Bind( ) const
{
    return m_tShaderConstantBufferPolicy.Bind( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::SetTexture

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::SetTexture(st_int32 nRegister, const TTextureClass& cTexture, st_bool bSubmitImmediately)
{
    return TShaderConstantBufferPolicy::SetTexture(nRegister, cTexture, bSubmitImmediately);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::SubmitSetTexturesInBatch

CShaderConstantBufferRI_TemplateList
inline void CShaderConstantBufferRI_t::SubmitSetTexturesInBatch(void)
{
    TShaderConstantBufferPolicy::SubmitSetTexturesInBatch( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::Load

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::Load(const char* pVertexFilename, 
                                          const char* pPixelFilename, 
                                          const SAppState& sAppState,
                                          const SRenderState& sRenderState)
{
    assert(pVertexFilename && strlen(pVertexFilename) > 0);
    assert(pPixelFilename && strlen(pPixelFilename) > 0);

    st_bool bSuccess = true;

    if (m_tShaderTechniquePolicy.LoadProgramBinary(pVertexFilename, pPixelFilename))
    {
        m_strVertexShaderName = pVertexFilename;
        m_strPixelShaderName = pPixelFilename;
        Report("  technique from program binary [%s & %s] OK", m_strVertexShaderName.NoPath( ).c_str( ), m_strPixelShaderName.NoPath( ).c_str( ));
    }
    else
    {
        // get file system pointer from Core lib
        CFileSystem* pFileSystem = CFileSystemInterface::Get( );
        assert(pFileSystem);

        // get vertex shader
        st_bool bFoundInVSCache = false;
        {
            // create the cache if first use
            if (!m_pVertexShaderCache)
                m_pVertexShaderCache = st_new(CVertexShaderCache, "CVertexShaderCache")(GFX_RESOURCE_VERTEX_SHADER, c_nShaderCacheInitialReserve);
            assert(m_pVertexShaderCache);
            m_strVertexShaderName = pVertexFilename;

            // cache lookup name
            #ifdef SHADER_CACHE_USES_FULL_PATHNAMES
                const CFixedString strCacheFilename = m_strVertexShaderName;
            #else
                CFixedString strCacheFilename = m_strVertexShaderName.NoPath( );
            #endif

            // is this vertex shader already cached?
            TVertexShader* pShader = m_pVertexShaderCache->Retrieve(strCacheFilename);
            if (!pShader)
            {
                // since a series of paths are scanned, check that the file exists first so that an error is registered only
                // when the load fails for other reasons
                if (pFileSystem->FileExists(m_strVertexShaderName.c_str( )))
                {
                    // wasn't in cache, so create it
                    if (m_tVertexShader.Load(m_strVertexShaderName.c_str( ), sAppState, sRenderState))
                    {
                        // and add to cache
                        const size_t c_siUnknownSize = 0;
                        m_pVertexShaderCache->Add(strCacheFilename, m_tVertexShader, c_siUnknownSize);
                    }
                    else
                        bSuccess = false;
                }
                else
                    bSuccess = false;
            }
            else
            {
                m_tVertexShader = *pShader;
                bFoundInVSCache = true;
            }
        }

        // get pixel shader
        st_bool bFoundInPSCache = false;
        {
            // create the cache if first use
            if (!m_pPixelShaderCache)
                m_pPixelShaderCache = st_new(CPixelShaderCache, "CPixelShaderCache")(GFX_RESOURCE_PIXEL_SHADER, c_nShaderCacheInitialReserve);
            assert(m_pPixelShaderCache);
            m_strPixelShaderName = pPixelFilename;

            // cache lookup name
            #ifdef SHADER_CACHE_USES_FULL_PATHNAMES
                const CFixedString strCacheFilename = m_strPixelShaderName;
            #else
                CFixedString strCacheFilename = m_strPixelShaderName.NoPath( );
            #endif

            // is this pixel shader already cached? 
            TPixelShader* pShader = m_pPixelShaderCache->Retrieve(strCacheFilename);
            if (!pShader)
            {
                // since a series of paths are scanned, check that the file exists first so that an error is registered only
                // when the load fails for other reasons
                if (pFileSystem->FileExists(m_strPixelShaderName.c_str( )))
                {
                    // wasn't in cache, so create it
                    if (m_tPixelShader.Load(m_strPixelShaderName.c_str( ), sAppState, sRenderState))
                    {
                        // and add to cache
                        const size_t c_siUnknownSize = 0;
                        m_pPixelShaderCache->Add(strCacheFilename, m_tPixelShader, c_siUnknownSize);
                    }
                    else
                        bSuccess = false;
                }
                else
                    bSuccess = false;
            }
            else
            {
                m_tPixelShader = *pShader;
                bFoundInPSCache = true;
            }
        }

        if (bSuccess)
        {
            bSuccess = m_tShaderTechniquePolicy.Link(m_tVertexShader, m_tPixelShader);

            if (bSuccess)
            {
                m_tShaderTechniquePolicy.SaveProgramBinary(pVertexFilename, pPixelFilename);

                if (!bFoundInVSCache || !bFoundInPSCache)
                    Report("  vsc: %d, psc: %d, technique [%s & %s] OK", 
                        m_pVertexShaderCache->Size( ), m_pPixelShaderCache->Size( ),
                        m_strVertexShaderName.NoPath( ).c_str( ), m_strPixelShaderName.NoPath( ).c_str( ));
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::ReleaseGfxResources

CShaderTechniqueRI_TemplateList
inline void CShaderTechniqueRI_t::ReleaseGfxResources(void)
{
    if (m_pVertexShaderCache)
    {
        #ifdef SHADER_CACHE_USES_FULL_PATHNAMES
            const CFixedString strCacheFilename = m_strVertexShaderName;
        #else
            CFixedString strCacheFilename = m_strVertexShaderName.NoPath( );
        #endif

        st_int32 nRefCount = m_pVertexShaderCache->Release(strCacheFilename);
        if (nRefCount == 0)
            m_tVertexShader.ReleaseGfxResources( );

        // delete cache if empty
        if (m_pVertexShaderCache->Size( ) == 0)
            st_delete<CVertexShaderCache>(m_pVertexShaderCache);
    }

    if (m_pPixelShaderCache)
    {
        #ifdef SHADER_CACHE_USES_FULL_PATHNAMES
            const CFixedString strCacheFilename = m_strPixelShaderName;
        #else
            CFixedString strCacheFilename = m_strPixelShaderName.NoPath( );
        #endif

        st_int32 nRefCount = m_pPixelShaderCache->Release(strCacheFilename);
        if (nRefCount == 0)
            m_tPixelShader.ReleaseGfxResources( );

        // delete cache if empty
        if (m_pPixelShaderCache->Size( ) == 0)
            st_delete<CPixelShaderCache>(m_pPixelShaderCache);
    }

    (void) m_tShaderTechniquePolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::GetCompiledShaderExtension

CShaderTechniqueRI_TemplateList
inline CFixedString CShaderTechniqueRI_t::GetCompiledShaderExtension(void)
{
    return TShaderTechniquePolicy::GetCompiledShaderExtension( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::GetCompiledShaderFolder

CShaderTechniqueRI_TemplateList
inline CFixedString CShaderTechniqueRI_t::GetCompiledShaderFolder(void)
{
    return TShaderTechniquePolicy::GetCompiledShaderFolder( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::VertexDeclNeedsInstancingAttribs

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::VertexDeclNeedsInstancingAttribs(void)
{
    return TShaderTechniquePolicy::VertexDeclNeedsInstancingAttribs( );
}

