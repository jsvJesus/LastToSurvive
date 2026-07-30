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
//  CShaderConstantBufferDirectX9::CShaderConstantBufferDirectX9

inline CShaderConstantBufferDirectX9::CShaderConstantBufferDirectX9( ) :
    m_siSizeOfDerivedClass(0),
    m_nRegister(-1),
    m_pBuffer(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::~CShaderConstantBufferDirectX9

inline CShaderConstantBufferDirectX9::~CShaderConstantBufferDirectX9( )
{
    st_delete_array<st_byte>(m_pBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::Init

inline st_bool CShaderConstantBufferDirectX9::Init(size_t siSizeOfDerivedClass, st_int32 nRegister)
{
    st_bool bSuccess = false;

    if (!m_pBuffer)
    {
        // copy input parameters
        m_siSizeOfDerivedClass = siSizeOfDerivedClass;
        m_nRegister = nRegister;

        m_pBuffer = st_new_array<st_byte>(siSizeOfDerivedClass, "DX9 constant buffer");

        bSuccess = (m_pBuffer != NULL);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::ReleaseGfxResources

inline void CShaderConstantBufferDirectX9::ReleaseGfxResources(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::Update

inline st_bool CShaderConstantBufferDirectX9::Update(void) const
{
    st_bool bSuccess = false;

    if (m_pBuffer)
    {
        // copy the layout content
        void* c_pConstBufSrc  = (void*) ((st_byte*) this + sizeof(CShaderConstantBufferDirectX9));

        memcpy(m_pBuffer, c_pConstBufSrc, m_siSizeOfDerivedClass);

        bSuccess = true;
    }
    else
        CCore::SetError("CShaderConstantBufferDirectX9::Update(), constant buffer wasn't successfully initialized");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferDirectX9::Bind

inline st_bool CShaderConstantBufferDirectX9::Bind(void) const
{
    st_bool bSuccess = false;

    if (m_pBuffer && m_nRegister != -1)
    {
        bSuccess = SUCCEEDED(DX9::Device( )->SetVertexShaderConstantF(c_asBufferRegisters[m_nRegister].m_nRegisterStart, (const st_float32*) m_pBuffer, UINT(c_asBufferRegisters[m_nRegister].m_nNumRegisters))) &&
                   SUCCEEDED(DX9::Device( )->SetPixelShaderConstantF(c_asBufferRegisters[m_nRegister].m_nRegisterStart, (const st_float32*) m_pBuffer, UINT(c_asBufferRegisters[m_nRegister].m_nNumRegisters)));
    }
    else
        CCore::SetError("CShaderConstantBufferDirectX9::Bind(), constant buffer wasn't successfully initialized");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::SetTexture

inline st_bool CShaderConstantBufferDirectX9::SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately)
{
    ST_UNREF_PARAM(bSubmitImmediately);

    return SUCCEEDED(DX9::Device( )->SetTexture(nRegister, cTexture.m_tTexturePolicy.GetTextureObject( )));
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX9::SubmitSetTexturesInBatch

inline void CShaderConstantBufferDirectX9::SubmitSetTexturesInBatch(void)
{
    // DX9 doesn't support texture batching, so they're always set immediate in SetTexture().
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CVertexShader::CVertexShader

inline CShaderTechniqueDirectX9::CVertexShader::CVertexShader( ) :
    m_pDx9Shader(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CVertexShader::Load

inline st_bool CShaderTechniqueDirectX9::CVertexShader::Load(const char* pFilename, const SAppState&, const SRenderState&)
{
    st_bool bSuccess = false;

    // get file system pointer from Core lib
    CFileSystem* pFileSystem = CFileSystemInterface::Get( );
    assert(pFileSystem);

    // set up temporary buffer for holding shader code
    const size_t c_siBufferSize = pFileSystem->FileSize(pFilename);
    if (c_siBufferSize > 0)
    {
        st_byte* pCompiledShaderCode = pFileSystem->LoadFile(pFilename);
        if (pCompiledShaderCode)
        {
            assert(pCompiledShaderCode);

            // create the vertex shader
            HRESULT hResult = DX9::Device( )->CreateVertexShader((const DWORD*) pCompiledShaderCode, &m_pDx9Shader);
            if (SUCCEEDED(hResult))
                bSuccess = true;
            else
                CCore::SetError("CShaderTechniqueDirectX9::CVertexShader::Load, DX9::Device( )->CreateVertexShader failed");

            pFileSystem->Release(pCompiledShaderCode);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CVertexShader::IsValid

inline st_bool CShaderTechniqueDirectX9::CVertexShader::IsValid(void) const
{
    return (m_pDx9Shader != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CVertexShader::ReleaseGfxResources

inline void CShaderTechniqueDirectX9::CVertexShader::ReleaseGfxResources(void)
{
    ST_SAFE_RELEASE(m_pDx9Shader);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CPixelShader::CPixelShader

inline CShaderTechniqueDirectX9::CPixelShader::CPixelShader( ) :
    m_pDx9Shader(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CPixelShader::Load

inline st_bool CShaderTechniqueDirectX9::CPixelShader::Load(const char* pFilename, const SAppState&, const SRenderState&)
{
    st_bool bSuccess = false;

    // get file system pointer from Core lib
    CFileSystem* pFileSystem = CFileSystemInterface::Get( );
    assert(pFileSystem);

    // set up temporary buffer for holding shader code
    const size_t c_siBufferSize = pFileSystem->FileSize(pFilename);
    if (c_siBufferSize > 0)
    {
        st_byte* pCompiledShaderCode = pFileSystem->LoadFile(pFilename);
        {
            assert(pCompiledShaderCode);

            // create the pixel shader
            HRESULT hResult = DX9::Device( )->CreatePixelShader((const DWORD*) pCompiledShaderCode, &m_pDx9Shader);
            if (SUCCEEDED(hResult))
                bSuccess = true;
            else
                CCore::SetError("CShaderTechniqueDirectX9::CPixelShader::Load, DX9::Device( )->CreatePixelShader failed");

            pFileSystem->Release(pCompiledShaderCode);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CPixelShader::IsValid

inline st_bool CShaderTechniqueDirectX9::CPixelShader::IsValid(void) const
{
    return (m_pDx9Shader != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::CPixelShader::ReleaseGfxResources

inline void CShaderTechniqueDirectX9::CPixelShader::ReleaseGfxResources(void)
{
    ST_SAFE_RELEASE(m_pDx9Shader);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::Link

inline st_bool CShaderTechniqueDirectX9::Link(const CVertexShader&, const CPixelShader&)
{
    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::Bind

inline st_bool CShaderTechniqueDirectX9::Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader, st_bool bDepthOnlyOpaqueMode) const
{
    st_bool bSuccess = true;

    // vertex shader
    #ifndef NDEBUG
        if (cVertexShader.IsValid( ))
        {
    #endif

            DX9::Device( )->SetVertexShader(cVertexShader.m_pDx9Shader);

    #ifndef NDEBUG
        }
        else
        {
            CCore::SetError("CShaderTechniqueDirectX9::Bind, vertex shader was not valid");
            bSuccess = false;
        }
    #endif

    // pixel shader
    #ifndef NDEBUG
        if (cPixelShader.IsValid( ))
        {
    #endif

            if (bDepthOnlyOpaqueMode)
                DX9::Device( )->SetPixelShader(NULL);
            else
                DX9::Device( )->SetPixelShader(cPixelShader.m_pDx9Shader);

            bSuccess = true;

    #ifndef NDEBUG
        }
        else
        {
            CCore::SetError("CShaderTechniqueDirectX9::Bind, pixel shader was not valid");
            bSuccess = false;
        }
    #endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::UnBind

inline void CShaderTechniqueDirectX9::UnBind(void)
{
    DX9::Device( )->SetVertexShader(NULL);
    DX9::Device( )->SetPixelShader(NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::ReleaseGfxResources

inline st_bool CShaderTechniqueDirectX9::ReleaseGfxResources(void)
{
    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::GetCompiledShaderExtension

inline CFixedString CShaderTechniqueDirectX9::GetCompiledShaderExtension(void)
{
    return "fx9obj";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::GetCompiledShaderFolder

inline CFixedString CShaderTechniqueDirectX9::GetCompiledShaderFolder(void)
{
    return "shaders_directx9";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX9::VertexDeclNeedsInstancingAttribs

inline st_bool CShaderTechniqueDirectX9::VertexDeclNeedsInstancingAttribs(void)
{
    return true;
}
