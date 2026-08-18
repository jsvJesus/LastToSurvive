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

#define SHADER_CAN_CACHE_PROGRAM_BINARIES_TO_DISK


///////////////////////////////////////////////////////////////////////  
//  Constants

const size_t c_siLinkedProgramSize = 4; // don't actually know how much space a linked program takes on the GPU


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::CShaderConstantBufferOpenGL

inline CShaderConstantBufferOpenGL::CShaderConstantBufferOpenGL( ) :
    m_uiBuffer(0),
    m_siSizeOfDerivedClass(0),
    m_uiRegister(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::~CShaderConstantBufferOpenGL

inline CShaderConstantBufferOpenGL::~CShaderConstantBufferOpenGL( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::Init

inline st_bool CShaderConstantBufferOpenGL::Init(size_t siSizeOfDerivedClass, st_int32 nRegister)
{
    if (!m_uiBuffer)
    {
        // copy input parameters
        m_siSizeOfDerivedClass = siSizeOfDerivedClass;
        m_uiRegister = nRegister;

        // generate empty constant buffer
        glGenBuffers(1, &m_uiBuffer);
    }

    return (m_uiBuffer != 0);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::ReleaseGfxResources

inline void CShaderConstantBufferOpenGL::ReleaseGfxResources(void)
{
    // intentionally empty
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::Update

inline st_bool CShaderConstantBufferOpenGL::Update(void) const
{
    st_bool bSuccess = false;

    if (m_uiBuffer != 0)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_uiBuffer);

        size_t siSizeofLayout = m_siSizeOfDerivedClass - sizeof(this);
        void* pMemPtr = (void*) ((st_byte*) this + sizeof(CShaderConstantBufferOpenGL));
        glBufferData(GL_UNIFORM_BUFFER, siSizeofLayout, pMemPtr, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::Bind

inline st_bool CShaderConstantBufferOpenGL::Bind(void) const
{
    st_bool bSuccess = false;

    if (m_uiBuffer != 0)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_uiBuffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_uiRegister, m_uiBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::SetTexture

inline st_bool CShaderConstantBufferOpenGL::SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately)
{
    ST_UNREF_PARAM(bSubmitImmediately);

    glActiveTexture(GL_TEXTURE0 + nRegister);
    glBindTexture(GL_TEXTURE_2D, cTexture.m_tTexturePolicy.GetTextureObject( ));
    glActiveTexture(GL_TEXTURE0);

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOpenGL::SubmitSetTexturesInBatch

inline void CShaderConstantBufferOpenGL::SubmitSetTexturesInBatch(void)
{
    // intentionally blank
}


///////////////////////////////////////////////////////////////////////
//  Helper: ShaderHadError

inline st_bool ShaderHadError(const char* pFilename, GLuint uiShader)
{
    assert(pFilename);

    st_bool bShaderError = false;

    st_int32 nCompileStatus = GL_FALSE;
    glGetShaderiv(uiShader, GL_COMPILE_STATUS, &nCompileStatus);

    bShaderError = (nCompileStatus == GL_FALSE);
    if (bShaderError)
    {
        st_int32 nInfoLogLength = 0;
        glGetShaderiv(uiShader, GL_INFO_LOG_LENGTH, &nInfoLogLength);

        if (nInfoLogLength > 0)
        {
            CStaticArray<st_char> aLogBuffer(nInfoLogLength + 1, "CShaderLoaderOpenGL::ShaderHadError");
            char* pLogBuffer = &aLogBuffer[0];

            st_int32 nCharsWritten = 0;
            glGetShaderInfoLog(uiShader, nInfoLogLength, &nCharsWritten, pLogBuffer);

            if (nCharsWritten > 0)
                CCore::SetError("In shader [%s], error: [%s]\n", pFilename, pLogBuffer);
        }
    }

    return bShaderError;
}


///////////////////////////////////////////////////////////////////////  
//  Helper: CompileShader

static const char* c_pNoInstancingMacro = "#define ST_OPENGL_NO_INSTANCING\n";
static const char* c_pGlslMacroVertex = "#version 330\n"
                                        "#define attribute in\n"
                                        "#define varying out\n"
                                        "#pragma optionNV(inline all)\n"
                                        "#pragma optionNV(unroll all)\n";

static const char* c_pGlslMacroFragment =   "#version 330\n"
                                            "#define varying in\n"
                                            "#pragma optionNV(inline all)\n"
                                            "#pragma optionNV(unroll all)\n";

static inline st_uint32 CompileShader(const char* pFilename, st_bool bVertexShader)
{
    assert(pFilename);

    st_uint32 uiShader = 0;

    // get file system pointer from Core lib
    CFileSystem* pFileSystem = CFileSystemInterface::Get( );
    assert(pFileSystem);

    // read the shader source code into a buffer
    size_t siFileSize = pFileSystem->FileSize(pFilename); // +1 for null terminator
    if (siFileSize > 0)
    {
        size_t siBufferSize = siFileSize + 1;

        // determine what macros are needed
        const st_bool c_bInstancingSupported = CGraphicsCapsOpenGL::IsInstancingSupported( );
        const st_bool c_bGlsl150 = CGraphicsCapsOpenGL::IsGlsl150OrHigher( );
        
        // macro insertion info
        const char* c_pGlsl150Macro = bVertexShader ? c_pGlslMacroVertex : c_pGlslMacroFragment;
        const size_t c_uiNoInstancingSize = strlen(c_pNoInstancingMacro);
        const size_t c_uiGlsl150Size = strlen(c_pGlsl150Macro);

        // expand buffer for macros, if needed
        if (!c_bInstancingSupported)
            siBufferSize += strlen(c_pNoInstancingMacro);
        if (c_bGlsl150)
            siBufferSize += strlen(c_pGlsl150Macro);

        // create temporary buffer
        CStaticArray<st_byte> aShaderBuffer(siBufferSize, "CompileShader");
        st_byte* pShaderBuffer = &aShaderBuffer[0];

        // read shader source file into buffer
        st_byte* pFileContents = pFileSystem->LoadFile(pFilename);

        if (pFileContents)
        {
            size_t uiMove = 0;
            if (!c_bInstancingSupported)
                uiMove += c_uiNoInstancingSize;
            if (c_bGlsl150)
                uiMove += c_uiGlsl150Size;

            memcpy(pShaderBuffer + uiMove, pFileContents, siFileSize); 
            pFileSystem->Release(pFileContents);

            // insert macros if necessary
            size_t uiInsertPoint = 0;
            if (c_bGlsl150)
            {
                memcpy(&aShaderBuffer[uiInsertPoint], c_pGlsl150Macro, c_uiGlsl150Size);
                uiInsertPoint += c_uiGlsl150Size;
            }

            if (!c_bInstancingSupported)
            {
                memcpy(&aShaderBuffer[uiInsertPoint], c_pNoInstancingMacro, c_uiNoInstancingSize);
                uiInsertPoint += c_uiNoInstancingSize;
            }

            // set null terminator
            pShaderBuffer[siBufferSize - 1] = '\0';

            // create the vertex shader
            if (bVertexShader)
                uiShader = glCreateShader(GL_VERTEX_SHADER);
            else
                uiShader = glCreateShader(GL_FRAGMENT_SHADER);

            // pass in shader source code
            const char* pShaderSourceCode = (const char*) pShaderBuffer;
            glShaderSource(uiShader, 1, &pShaderSourceCode, NULL);

            // compile
            glCompileShader(uiShader);

            if (ShaderHadError(pFilename, uiShader))
                uiShader = 0;
        }
    }

    return uiShader;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CVertexShader::CVertexShader

inline CShaderTechniqueOpenGL::CVertexShader::CVertexShader( ) :
    m_uiShader(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CVertexShader::Load

inline st_bool CShaderTechniqueOpenGL::CVertexShader::Load(const char* pFilename, const SAppState& /*sAppState*/, const SRenderState& /*sRenderState*/)
{
    m_uiShader = CompileShader(pFilename, true);
    return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CVertexShader::IsValid

inline st_bool CShaderTechniqueOpenGL::CVertexShader::IsValid(void) const
{
    return (m_uiShader != 0);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CVertexShader::ReleaseGfxResources

inline st_bool CShaderTechniqueOpenGL::CVertexShader::ReleaseGfxResources(void)
{
    st_bool bSuccess = false;

    if (m_uiShader != 0)
    {
        glDeleteShader(m_uiShader);
        m_uiShader = 0;

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CPixelShader::CPixelShader

inline CShaderTechniqueOpenGL::CPixelShader::CPixelShader( ) :
    m_uiShader(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CPixelShader::Load

inline st_bool CShaderTechniqueOpenGL::CPixelShader::Load(const char* pFilename, const SAppState& /*sAppState*/, const SRenderState& /*sRenderState*/)
{
    m_uiShader = CompileShader(pFilename, false);
    return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CPixelShader::IsValid

inline st_bool CShaderTechniqueOpenGL::CPixelShader::IsValid(void) const
{
    return (m_uiShader != 0);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CPixelShader::ReleaseGfxResources

inline st_bool CShaderTechniqueOpenGL::CPixelShader::ReleaseGfxResources(void)
{
    st_bool bSuccess = false;

    if (m_uiShader != 0)
    {
        glDeleteShader(m_uiShader);
        m_uiShader = 0;

        bSuccess = true;
    }   

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::CShaderTechniqueOpenGL

inline CShaderTechniqueOpenGL::CShaderTechniqueOpenGL( ) :
    m_uiProgram(0)
{
    memset(m_szLinkCacheFilename, 0, c_siFixedStringDefaultLength);
}


///////////////////////////////////////////////////////////////////////
//  BindVertexAttributes

inline void BindVertexAttributes(GLuint uiProgram)
{
    const char* c_apSlotNames[ ] = 
    {
        "in_vSlot0",
        "in_vSlot1",
        "in_vSlot2",
        "in_vSlot3",
        "in_vSlot4",
        "in_vSlot5",
        "in_vSlot6",
        "in_vSlot7",
        "in_vSlot8",
        "in_vSlot9",
        "in_vSlot10",
        "in_vSlot11",
        "in_vSlot12",
        "in_vSlot13",
        "in_vSlot14",
        "in_vSlot15"
    };

    const st_int32 c_nNumSlots = sizeof(c_apSlotNames) / sizeof(c_apSlotNames[0]);
    for (st_int32 i = 0; i < c_nNumSlots; ++i)
        glBindAttribLocation(uiProgram, i, c_apSlotNames[i]);
}


///////////////////////////////////////////////////////////////////////
//  BindTextureSamplers

inline void BindTextureSamplers(GLuint uiProgram)
{
    glUseProgram(uiProgram);
    {
        struct SSamplerBinding
        {
            const char*             m_pName;
            EForestTextureRegister  m_eRegister;
        };

        const SSamplerBinding c_asSpeedTreeSamplers[ ] =
        {
            // tree/grass textures
            { "DiffuseMap",                     TEXTURE_REGISTER_DIFFUSE },
            { "NormalMap",                      TEXTURE_REGISTER_NORMAL },
            { "DetailDiffuseMap",               TEXTURE_REGISTER_DETAIL },
            { "DetailNormalMap",                TEXTURE_REGISTER_DETAIL_NORMAL },
            { "SpecularMaskMap",                TEXTURE_REGISTER_SPECULAR_MASK },
            { "TransmissionMaskMap",            TEXTURE_REGISTER_TRANSMISSION_MASK },
            { "NoiseMap",                       TEXTURE_REGISTER_FIZZLE_NOISE },
            { "PerlinNoiseKernel",              TEXTURE_REGISTER_PERLIN_NOISE_KERNEL },
            { "ImageBasedAmbientLightingMap",   TEXTURE_REGISTER_IMAGE_BASED_AMBIENT_LIGHTING },
            { "Overlay",                        TEXTURE_REGISTER_DIFFUSE },
            { "Clouds",                         TEXTURE_REGISTER_DIFFUSE },

            // shadow maps
            { "ShadowMap0",                     TEXTURE_REGISTER_SHADOW_MAP_0 },
            { "ShadowMap1",                     TEXTURE_REGISTER_SHADOW_MAP_1 },
            { "ShadowMap2",                     TEXTURE_REGISTER_SHADOW_MAP_2 },
            { "ShadowMap3",                     TEXTURE_REGISTER_SHADOW_MAP_3 },

            // terrain textures
            { "TerrainSplatMap",                TEXTURE_REGISTER_TERRAIN_SPLAT },
            { "TerrainNormalMap",               TEXTURE_REGISTER_TERRAIN_NORMAL },
            { "TerrainSplatLayerMap0",          TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_0 },
            { "TerrainSplatLayerMap1",          TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_1 },
            { "TerrainSplatLayerMap2",          TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_2 },

            // deferred rendering
            { "RenderTarget0",                  TEXTURE_REGISTER_DIFFUSE },         // sampler 0
            { "RenderTarget1",                  TEXTURE_REGISTER_NORMAL },          // sampler 1
            { "RenderTargetDepth",              TEXTURE_REGISTER_DETAIL },          // sampler 2

            // terminator
            { NULL,                             TEXTURE_REGISTER_COUNT }
        };

        const SSamplerBinding* pSampler = c_asSpeedTreeSamplers;
        while (pSampler->m_pName)
        {
            GLint nLocation = glGetUniformLocation(uiProgram, pSampler->m_pName);
            if (nLocation > -1)
                glUniform1i(nLocation, pSampler->m_eRegister);
            // don't issue warning if not found; no single shader will have all of the texture names in the table

            ++pSampler;
        }
    }
    glUseProgram(0);
}


///////////////////////////////////////////////////////////////////////
//  BindUbos

inline void BindUbos(GLuint uiProgram)
{
    glUseProgram(uiProgram);
    {
        struct SUboBinding
        {
            const char*     m_pName;
            GLuint          m_uiBindingIndex;
        };

        const SUboBinding c_asSpeedTreeSamplers[ ] =
        {
            { "SFrameCBLayout",         ST_CONST_BUF_REGISTER_FRAME },
            { "SBaseTreeCBLayout",      ST_CONST_BUF_REGISTER_BASE_TREE },
            { "SInstancingCBLayout",    ST_CONST_BUF_REGISTER_INSTANCING },
            { "SMaterialCBLayout",      ST_CONST_BUF_REGISTER_MATERIAL },
            { "SWindDynamicsCBLayout",  ST_CONST_BUF_REGISTER_WIND_DYNAMICS },
            { "SFogAndSkyCBLayout",     ST_CONST_BUF_REGISTER_FOG_AND_SKY },
            { "STerrainCBLayout",       ST_CONST_BUF_REGISTER_TERRAIN },
            { "SBloomCBLayout",         ST_CONST_BUF_REGISTER_BLOOM },

            // terminator
            { NULL,                     ST_CONST_BUF_REGISTER_COUNT }
        };

        const SUboBinding* pUbo = c_asSpeedTreeSamplers;
        while (pUbo->m_pName)
        {
            GLuint uiLocation = glGetUniformBlockIndex(uiProgram, pUbo->m_pName);
            if (uiLocation != GL_INVALID_INDEX)
                glUniformBlockBinding(uiProgram, uiLocation, pUbo->m_uiBindingIndex);
            else
            {
                // no need to generate an error or warning; some GLSL compilers will remove unused UBOs
                //CCore::SetError("OpenGL Renderer::BindUbos() could not find UBO by name [%s]", pUbo->m_pName);
            }

            ++pUbo;
        }
    }
    glUseProgram(0);
}


///////////////////////////////////////////////////////////////////////
//  ProgramHadError

inline st_bool ProgramHadError(GLuint uiProgram)
{
    st_bool bProgramError = false;

    st_int32 nLinkStatus = GL_FALSE;
    glGetProgramiv(uiProgram, GL_LINK_STATUS, &nLinkStatus);

    bProgramError = (nLinkStatus == GL_FALSE);
    if (bProgramError)
    {
        st_int32 nInfoLogLength = 0;
        glGetProgramiv(uiProgram, GL_INFO_LOG_LENGTH, &nInfoLogLength);

        if (nInfoLogLength > 0)
        {
            CStaticArray<st_char> aLogBuffer(nInfoLogLength, "ProgramHadError");
            char* pLogBuffer = &aLogBuffer[0];

            st_int32 nCharsWritten = 0;
            glGetProgramInfoLog(uiProgram, nInfoLogLength, &nCharsWritten, pLogBuffer);

            if (nCharsWritten > 0)
                CCore::SetError("Error in GLSL program: [\n%s]", pLogBuffer);
        }
    }

    return bProgramError;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::PrepareProgram

inline void CShaderTechniqueOpenGL::PrepareProgram(void)
{
    BindUbos(m_uiProgram);
    BindTextureSamplers(m_uiProgram);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::LoadProgramBinary

inline st_bool CShaderTechniqueOpenGL::LoadProgramBinary(const char* pVertexFilename, const char* pPixelFilename)
{
    st_bool bReturn = false;

    // create the linked program cache if first use
    if (!m_pLinkedProgramCache)
        m_pLinkedProgramCache = st_new(CLinkedProgramCache, "CLinkedProgramCache")(GFX_RESOURCE_OTHER, c_nLinkedProgramCacheInitialReserve);
    assert(m_pLinkedProgramCache);

    // check the cache first
    CFixedString strLinkCacheFilename = CFixedString(pVertexFilename).NoExtension( ) + "_" + CFixedString(pPixelFilename).NoPath( ).NoExtension( ) + ".bin";
    assert(strLinkCacheFilename.size( ) < c_siFixedStringDefaultLength);
    strcpy(m_szLinkCacheFilename, strLinkCacheFilename.c_str( ));
    GLuint* pCached = m_pLinkedProgramCache->Retrieve(strLinkCacheFilename);
    if (pCached != NULL)
    {
        m_uiProgram = *pCached;
        PrepareProgram( );
        bReturn = true;
    }

    // check if there is a precompiled shader, if we can use it
    #ifdef SHADER_CAN_CACHE_PROGRAM_BINARIES_TO_DISK
        if (!bReturn && glProgramBinary != NULL)
        {
            // get file system pointer from Core lib
            CFileSystem* pFileSystem = CFileSystemInterface::Get( );
            assert(pFileSystem);

            // check if file exists and is newer than both of the shader sources
            if (pFileSystem->FileExists(strLinkCacheFilename.c_str( )) &&
                pFileSystem->CompareFileTimes(strLinkCacheFilename.c_str( ), pVertexFilename) == CFileSystem::SECOND_OLDER &&
                pFileSystem->CompareFileTimes(strLinkCacheFilename.c_str( ), pPixelFilename) == CFileSystem::SECOND_OLDER)
            {
                st_byte* pFileContents = pFileSystem->LoadFile(strLinkCacheFilename.c_str( ));
                if (pFileContents != NULL)
                {
                    st_byte* pCurrentPos = pFileContents;
                    GLenum eProgramFormat = *((GLenum*)(pCurrentPos));
                    pCurrentPos += sizeof(GLenum);

                    st_int32 nProgramSize = *((st_int32*)(pCurrentPos));
                    pCurrentPos += sizeof(st_int32);

                    // search the available formats to see if it matches
                    const st_uint32 c_uiMaxSupportedFormats = 10;
                    st_int32 nNumSupportedFormats = 0;
                    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &nNumSupportedFormats);
                    assert(nNumSupportedFormats < st_int32(c_uiMaxSupportedFormats));
                    GLenum aSupportedFormats[c_uiMaxSupportedFormats] = { 0 };
                    glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, (GLint*)&aSupportedFormats);
                    for (st_uint32 i = 0; i < c_uiMaxSupportedFormats; ++i)
                    {
                        if (aSupportedFormats[i] == eProgramFormat || aSupportedFormats[i] == 0)
                        {
                            eProgramFormat = aSupportedFormats[i];
                            break;
                        }
                    }

                    if (eProgramFormat != 0 && nProgramSize > 0)
                    {
                        m_uiProgram = glCreateProgram( );

                        BindVertexAttributes(m_uiProgram);
        
                        glProgramBinary(m_uiProgram, eProgramFormat, pCurrentPos, nProgramSize);
                        
                        st_int32 nLinkStatus = GL_FALSE;
                        glGetProgramiv(m_uiProgram, GL_LINK_STATUS, &nLinkStatus);

                        if (nLinkStatus == GL_TRUE)
                        {
                            PrepareProgram( );
                            m_pLinkedProgramCache->Add(strLinkCacheFilename, m_uiProgram, c_siLinkedProgramSize);
                            bReturn = true;
                        }
                        else
                        {
                            glDeleteProgram(m_uiProgram);
                            m_uiProgram = 0;
                        }
                    }

                    pFileSystem->Release(pFileContents);
                }
            }
        }
    #endif

    return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::SaveProgramBinary

inline st_bool CShaderTechniqueOpenGL::SaveProgramBinary(const char* pVertexFilename, const char* pPixelFilename)
{
    st_bool bReturn = false;

    CFixedString strPreLinkedFilename = CFixedString(pVertexFilename).NoExtension( ) + "_" + CFixedString(pPixelFilename).NoPath( ).NoExtension( ) + ".bin";
            
    // save compiled shader for future use
    #ifdef SHADER_CAN_CACHE_PROGRAM_BINARIES_TO_DISK
        if (glGetProgramBinary != NULL)
        {
            st_int32 nProgramSize = 0;
            glGetProgramiv(m_uiProgram, GL_PROGRAM_BINARY_LENGTH, &nProgramSize);

            if (nProgramSize > 0)
            {
                CStaticArray<st_byte> aData(nProgramSize, "CShaderTechniqueOpenGL::SavePrelinkedShader");
                st_byte* pData = &aData[0];

                GLenum eProgramFormat;
                glGetProgramBinary(m_uiProgram, nProgramSize, NULL, &eProgramFormat, pData);

                FILE* pFile = fopen(strPreLinkedFilename.c_str( ), "wb");
                if (pFile != NULL)
                {
                    fwrite(&eProgramFormat, sizeof(GLenum), 1, pFile);
                    fwrite(&nProgramSize, sizeof(st_int32), 1, pFile);
                    fwrite(pData, nProgramSize, 1, pFile);
                    fclose(pFile);
                    
                    bReturn = true;
                    printf("   saved program binary [%s]\n", strPreLinkedFilename.NoPath( ).c_str( ));
                }
            }
        }
    #endif

    // add this program to the cache
    m_pLinkedProgramCache->Add(strPreLinkedFilename, m_uiProgram, c_siLinkedProgramSize);

    return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::Link

inline st_bool CShaderTechniqueOpenGL::Link(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader)
{
    st_bool bSuccess = false;

    if (cVertexShader.IsValid( ) && cPixelShader.IsValid( ))
    {
        m_uiProgram = glCreateProgram( );

        glAttachShader(m_uiProgram, cVertexShader.m_uiShader);
        glAttachShader(m_uiProgram, cPixelShader.m_uiShader);

        BindVertexAttributes(m_uiProgram);

        if (glProgramParameteri != NULL)
            glProgramParameteri(m_uiProgram, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        glLinkProgram(m_uiProgram);

        bSuccess = !ProgramHadError(m_uiProgram);

        if (bSuccess)
        {
            PrepareProgram( );
            bSuccess = true;
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::Bind

inline st_bool CShaderTechniqueOpenGL::Bind(const CVertexShader&, const CPixelShader&) const
{
    st_bool bSuccess = false;

    if (m_uiProgram != 0)
    {
        glUseProgram(m_uiProgram);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::UnBind

inline void CShaderTechniqueOpenGL::UnBind(void)
{
    glUseProgram(0);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::ReleaseGfxResources

inline st_bool CShaderTechniqueOpenGL::ReleaseGfxResources(void)
{
    st_bool bSuccess = false;

    if (m_uiProgram != 0 && m_pLinkedProgramCache)
    {
        st_int32 nRefCount = m_pLinkedProgramCache->Release(m_szLinkCacheFilename);
        if (nRefCount == 0)
        {
            glDeleteProgram(m_uiProgram);
        }

        m_uiProgram = 0;

        // delete cache if empty
        if (m_pLinkedProgramCache->Size( ) == 0)
            st_delete<CLinkedProgramCache>(m_pLinkedProgramCache);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::GetCompiledShaderExtension

inline CFixedString CShaderTechniqueOpenGL::GetCompiledShaderExtension(void)
{
    return "glsl";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::GetCompiledShaderFolder

inline CFixedString CShaderTechniqueOpenGL::GetCompiledShaderFolder(void)
{
    return "shaders_opengl_glsl";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOpenGL::VertexDeclNeedsInstancingAttribs

inline st_bool CShaderTechniqueOpenGL::VertexDeclNeedsInstancingAttribs(void)
{
    return false;
}
