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


////////////////////////////////////////////////////////////
// CRenderStateRI_t::CRenderStateRI

CRenderStateRI_TemplateList
inline CRenderStateRI_t::CRenderStateRI( ) :
    m_bFallbackTexturesInited(false)
{
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::~CRenderStateRI

CRenderStateRI_TemplateList
inline CRenderStateRI_t::~CRenderStateRI( )
{
}


////////////////////////////////////////////////////////////
//  Helper function: StringToLongHash

inline st_int64 StringToLongHash(const CFixedString& strInput)
{
    st_int64 lHash = 0;

    for (size_t i = 0; i < strInput.length( ); ++i)
        lHash = strInput[i] + (lHash << 6) + (lHash << 16) - lHash;

    return lHash;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::InitGfx

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitGfx(const SAppState& sAppState,
                                         const CArray<CFixedString>& aSearchPaths,
                                         st_int32 nMaxAnisotropy,
                                         st_float32 fTextureAlphaScalar,
                                         const CFixedString& strVertexShaderBaseName,
                                         const CFixedString& strPixelShaderBaseName,
                                         const CWind* pWind)
{
    // init textures
    st_bool bSuccess = LoadTextures(aSearchPaths, nMaxAnisotropy);

    // init constant buffer
    bSuccess &= InitConstantBuffer(sAppState, fTextureAlphaScalar, pWind);

    // init state block
    bSuccess &= m_cStateBlock.Init(sAppState, *this);

    // init shaders
    bSuccess &= LoadShaders(aSearchPaths, strVertexShaderBaseName, strPixelShaderBaseName);

    // load fallback textures if not yet done (shared by all render state objects)
    bSuccess &= InitFallbackTextures( );
    m_bFallbackTexturesInited = true;

    // set sort key value based on unique shader pair
    CFixedString strHashKey = CFixedString("v") + strVertexShaderBaseName + CFixedString("p") + strPixelShaderBaseName + CFixedString::Format("%d", m_eFaceCulling);
    m_lSortKey = StringToLongHash(strHashKey);

    return bSuccess;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::ReleaseGfxResources

CRenderStateRI_TemplateList
inline void CRenderStateRI_t::ReleaseGfxResources(void)
{
    // release shaders
    m_cTechnique.ReleaseGfxResources( );

    // release state block
    m_cStateBlock.ReleaseGfxResources( );

    // release textures
    for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
        m_atTextureObjects[i].ReleaseGfxResources( );

    // release fallback textures if last refcount
    if (m_bFallbackTexturesInited && --m_nFallbackTextureRefCount == 0)
    {
        for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
            if (m_atFallbackTextures[i].IsValid( ))
                m_atFallbackTextures[i].ReleaseGfxResources( );
    }

    m_sConstantBuffer.ReleaseGfxResources( );
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::GetTechnique

CRenderStateRI_TemplateList
inline const TShaderTechniqueClass& CRenderStateRI_t::GetTechnique(void) const
{
    return m_cTechnique;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::GetStateBlock

CRenderStateRI_TemplateList
inline const TStateBlockClass& CRenderStateRI_t::GetStateBlock(void) const
{
    return m_cStateBlock;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::BindConstantBuffer

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindConstantBuffer(void) const
{
    return m_sConstantBuffer.Bind( );
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::BindShader

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindShader(void) const
{
    return m_cTechnique.Bind( );
}


//////////////////////////////////////////////////////////// 
//  CRenderStateRI_t::BindTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindTextures(ERenderPass ePass, ETextureBindMode eTextureBindMode) const
{
    st_bool bSuccess = true;

    if (eTextureBindMode != TEXTURE_BIND_DISABLED)
    {
        // always bind diffuse texture
        const TTextureClass& cTextureToBind = ((eTextureBindMode == TEXTURE_BIND_ENABLED) && m_atTextureObjects[TL_DIFFUSE].IsValid( )) ? m_atTextureObjects[TL_DIFFUSE] : m_atFallbackTextures[TL_DIFFUSE];
        bSuccess &= BindTexture(TL_DIFFUSE, cTextureToBind);

        // only bind the render of the layers if lighting pass is used (diffuse is always needed in case of alpha transparency)
        if (ePass == RENDER_PASS_MAIN)
        {
            // change TL_AUX_ATLAS1 to TL_NUM_TEX_LAYERS is TL_AUX_ATLAS1 & TL_AUX_ATLAS2 are to be used; note that in GraphicsApiAbstraction.h, the fizzle and perlin
            // noise filters used the same registers as TL_AUX_ATLAS1 and TL_AUX_ATLAS2 and would have to be updated

            //for (st_int32 nLayer = TL_DIFFUSE + 1; nLayer < TL_NUM_TEX_LAYERS; ++nLayer) 
            for (st_int32 nLayer = TL_DIFFUSE + 1; nLayer < TL_AUX_ATLAS1; ++nLayer) // change TL_AUX_ATLAS1 to TL_NUM_TEX_LAYERS is TL_AUX_ATLAS1 & TL_AUX_ATLAS2 are to be used
            {
                const TTextureClass& cNonDiffuseTextureToBind = ((eTextureBindMode == TEXTURE_BIND_ENABLED) && m_atTextureObjects[nLayer].IsValid( )) ? m_atTextureObjects[nLayer] : m_atFallbackTextures[nLayer];
                bSuccess &= BindTexture(nLayer, cNonDiffuseTextureToBind);
            }
        }       

        TShaderConstantBufferClass::SubmitSetTexturesInBatch( );
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::BindStateBlock

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindStateBlock(void) const
{
    return m_cStateBlock.Bind( );
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::Bind

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindMaterialWhole(ERenderPass ePass, ETextureBindMode eTextureBindMode) const
{
    return BindTextures(ePass, eTextureBindMode) && 
           BindConstantBuffer( ) &&
           BindShader( ) && 
           BindStateBlock( );
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::UnBind

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::UnBind(void)
{
    return TShaderTechniqueClass::UnBind( );
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::LoadTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::LoadTextures(const CArray<CFixedString>& aSearchPaths, st_int32 nMaxAnisotropy)
{
    st_bool bSuccess = true;

    for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
    {
        // if texture is blank, don't attempt to load
        const char* pFilename = m_apTextures[i];
        if (pFilename && strlen(pFilename) > 0)
        {
            if (!InitTexture(pFilename, m_atTextureObjects[i], aSearchPaths, nMaxAnisotropy))
                CCore::SetError("Failed to load texture [%s]; using fallback", pFilename);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CRenderStateRI_t::InitTexture

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitTexture(const char* pFilename,
                                             TTextureClass& tTextureObject,
                                             const CArray<CFixedString>& aSearchPaths, 
                                             st_int32 nMaxAnisotropy)
{
    st_bool bSuccess = false;

    if (pFilename && strlen(pFilename) > 0)
    {
        bSuccess = true;

        // look for the texture at each location in the search path
        st_bool bFound = false;
        for (st_int32 nSearchPath = 0; nSearchPath < st_int32(aSearchPaths.size( )) && !bFound; ++nSearchPath)
        {
            // build complete filename, adjusting for per-platform requirements
            CFixedString strSearchLocation = CFileSystem::CleanPlatformFilename(aSearchPaths.at(nSearchPath) + c_szFolderSeparator + CFixedString(pFilename).NoPath( ));
            
            // if the Load() call succeeds, the texture was found
            if (tTextureObject.Load(strSearchLocation.c_str( ), nMaxAnisotropy))
            {
                bFound = true;
                break;
            }
        }

        bSuccess &= bFound;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CRenderStateRI_t::InitConstantBuffer

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitConstantBuffer(const SAppState& sAppState, st_float32 fTextureAlphaScalar, const CWind* pWind)
{
    st_bool bSuccess = false;

    if (m_sConstantBuffer.Init(sizeof(m_sConstantBuffer), ST_CONST_BUF_REGISTER_MATERIAL))
    {
        const Vec3 c_vOneThird(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);

        m_sConstantBuffer.u_vAmbientColor = m_vAmbientColor;
        m_sConstantBuffer.u_vDiffuseColor = m_vDiffuseColor * m_fDiffuseScalar;
        m_sConstantBuffer.u_vSpecularColor = m_vSpecularColor;
        m_sConstantBuffer.u_vTransmissionColor = (m_vTransmissionColor - c_vOneThird) * 3.0f;
        m_sConstantBuffer.u_fShininess = m_fShininess;
        m_sConstantBuffer.u_fBranchSeamWeight = m_fBranchSeamWeight;
        m_sConstantBuffer.u_fOneMinusAmbientContrastFactor = 1.0f - m_fAmbientContrastFactor;
        m_sConstantBuffer.u_fTransmissionShadowBrightness = m_fTransmissionShadowBrightness;
        m_sConstantBuffer.u_fTransmissionViewDependency = m_fTransmissionViewDependency;

        if (sAppState.m_bAlphaToCoverage)
        {
            // m_fAlphaScalar = alpha scalar (controls fullness of transparent textures; tuned by artist in Modeler)
            m_sConstantBuffer.u_fAlphaScalar = m_fAlphaScalar;
        }
        else 
        {
            // m_fAlphaScalar = alpha scalar (controls fullness of transparent textures; tuned by artist in Modeler)
            m_sConstantBuffer.u_fAlphaScalar = m_fAlphaScalar * fTextureAlphaScalar;
        }

        // effect flags
        //
        // this table must match the c_asEffects table in ShaderSourceGenerator.cpp in the SRT Exporter source
        const bool c_bPerVertexLighting = (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX) || (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);
        const bool c_bPerPixelLighting = (m_eLightingModel == LIGHTING_MODEL_PER_PIXEL) || (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);
        const bool c_bLightingTransition = (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);

        float* pEffectFlag = &(m_sConstantBuffer.u_avEffectConfigFlags[0].x);

        // FORWARD_LIGHTING_PER_VERTEX
        *pEffectFlag++ = st_float32(c_bPerVertexLighting);

        // FORWARD_LIGHTING_PER_PIXEL   
        *pEffectFlag++ = st_float32(c_bPerPixelLighting);

        // FORWARD_LIGHTING_TRANSITION
        *pEffectFlag++ = st_float32(c_bLightingTransition);

        // AMBIENT_OCCLUSION
        *pEffectFlag++ = st_float32(m_bAmbientOcclusion);

        // AMBIENT_CONTRAST         
        *pEffectFlag++ = st_float32(m_eAmbientContrast);

        // DETAIL_LAYER             
        *pEffectFlag++ = st_float32(m_eDetailLayer);

        // DETAIL_NORMAL_LAYER
        *pEffectFlag++ = st_float32(m_eDetailLayer != EFFECT_OFF && IsTextureLayerPresent(TL_DETAIL_DIFFUSE));

        // SPECULAR                 
        *pEffectFlag++ = st_float32(m_eSpecular);

        // TRANSMISSION             
        *pEffectFlag++ = st_float32(m_eTransmission);

        // BRANCH_SEAM_SMOOTHING        
        *pEffectFlag++ = st_float32(m_eBranchSeamSmoothing);

        // SMOOTH_LOD                   
        *pEffectFlag++ = (m_eLodMethod == LOD_METHOD_SMOOTH ? 1.0f : 0.0f);

        // FADE_TO_BILLBOARD
        *pEffectFlag++ = st_float32(m_bFadeToBillboard ? 1.0f : 0.0f);

        // HAS_HORZ_BB
        *pEffectFlag++ = st_float32(m_bHorzBillboard);

        // BACKFACE_CULLING         
        *pEffectFlag++ = st_float32(m_eFaceCulling);

        // AMBIENT_IMAGE_LIGHTING       
        *pEffectFlag++ = st_float32(m_eAmbientImageLighting);

        // HUE_VARIATION                
        *pEffectFlag++ = st_float32(m_eHueVariation);

        // SHADOW_SMOOTHING         
        *pEffectFlag++ = st_float32(m_bShadowSmoothing ? 1.0f : 0.0f);

        // DIFFUSE_MAP_OPAQUE           
        *pEffectFlag++ = st_float32(m_bDiffuseAlphaMaskIsOpaque ? 1.0f : 0.0f);

        assert((pEffectFlag - &m_sConstantBuffer.u_avEffectConfigFlags[0].x) <= ST_NUM_EFFECT_CONFIG_FLOAT4S * 4);

        // various wind flags
        if (pWind)
        {
            // wind config flags
            float* pWindConfigFlag = &(m_sConstantBuffer.u_avWindConfigFlags[0].x);
            // 0
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::GLOBAL_WIND)                    && IsGlobalWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::GLOBAL_PRESERVE_SHAPE)          && IsGlobalWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_SIMPLE_1)                && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_1)           && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            // 1
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_FROND_1)     && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_TURBULENCE_1)            && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_WHIP_1)                  && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_OSC_COMPLEX_1)           && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            // 2
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_SIMPLE_2)                && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_2)           && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_FROND_2)     && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_TURBULENCE_2)            && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            // 3
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_WHIP_2)                  && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_OSC_COMPLEX_2)           && IsBranchWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_VERTEX_NORMAL_1)    && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_COMPUTED_1)         && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            // 4
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TUMBLE_1)                  && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TWITCH_1)                  && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_OCCLUSION_1)               && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_VERTEX_NORMAL_2)    && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            // 5
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_COMPUTED_2)         && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TUMBLE_2)                  && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TWITCH_2)                  && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_OCCLUSION_2)               && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            // 6
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_ONE_SIDED)         && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_TWO_SIDED)         && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_ADJUST_LIGHTING)   && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            *pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::ROLLING)                        && IsFullWindEnabled( )) ? 1.0f: 0.0f;
            assert((pWindConfigFlag - &m_sConstantBuffer.u_avWindConfigFlags[0].x) <= ST_NUM_WIND_CONFIG_FLOAT4S * 4);

            // wind LOD options
            float* pWindLodFlag = &(m_sConstantBuffer.u_avWindLodFlags[0].x);
            *pWindLodFlag++ = IsGlobalWindEnabled( )                    ? 1.0f : 0.0f;  // ST_WIND_LOD_GLOBAL
            *pWindLodFlag++ = IsBranchWindEnabled( )                    ? 1.0f : 0.0f;  // ST_WIND_LOD_BRANCH
            *pWindLodFlag++ = IsFullWindEnabled( )                      ? 1.0f : 0.0f;  // ST_WIND_LOD_FULL
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_GLOBAL)    ? 1.0f : 0.0f;  // ST_WIND_LOD_NONE_X_GLOBAL
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_BRANCH)    ? 1.0f : 0.0f;  // ST_WIND_LOD_NONE_X_BRANCH
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_FULL)      ? 1.0f : 0.0f;  // ST_WIND_LOD_NONE_X_FULL
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_GLOBAL_X_BRANCH)  ? 1.0f : 0.0f;  // ST_WIND_LOD_GLOBAL_X_BRANCH
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_GLOBAL_X_FULL)    ? 1.0f : 0.0f;  // ST_WIND_LOD_GLOBAL_X_FULL
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_BRANCH_X_FULL)    ? 1.0f : 0.0f;  // ST_WIND_LOD_BRANCH_X_FULL
            *pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE)             ? 1.0f : 0.0f;  // ST_WIND_LOD_NONE

            // is full wind in transition? if so, set flag for rolling wind to fade with it
            *pWindLodFlag++ = ((m_eWindLod == WIND_LOD_NONE_X_FULL) || 
                               (m_eWindLod == WIND_LOD_GLOBAL_X_FULL) ||
                               (m_eWindLod == WIND_LOD_BRANCH_X_FULL)) ? 1.0f : 0.0f;   // ST_WIND_LOD_ROLLING_FADE

            // is branch wind on at all?
            *pWindLodFlag++ = IsBranchWindEnabled( );

            assert((pWindLodFlag - &m_sConstantBuffer.u_avWindLodFlags[0].x) <= ST_NUM_WIND_LOD_FLOAT4S * 4);
        }
        else
        {
            memset(m_sConstantBuffer.u_avWindConfigFlags, 0, sizeof(m_sConstantBuffer.u_avWindConfigFlags));
            memset(m_sConstantBuffer.u_avWindLodFlags, 0, sizeof(m_sConstantBuffer.u_avWindLodFlags));
        }

        bSuccess = m_sConstantBuffer.Update( );
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::BindTexture

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindTexture(st_int32 nLayer, const TTextureClass& tTexture) const
{
    st_bool bSuccess = false;

    if (tTexture.IsValid( ))
    {
        bSuccess = true;

        if (tTexture != m_atLastBoundTextures[nLayer])
        {
            bSuccess = TShaderConstantBufferClass::SetTexture(TEXTURE_REGISTER_DIFFUSE + nLayer, tTexture, false);
            m_atLastBoundTextures[nLayer] = tTexture;
        }
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::LoadShaders

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::LoadShaders(const CArray<CFixedString>& aSearchPaths,
                                             const CFixedString& strVertexShaderBaseName,
                                             const CFixedString& strPixelShaderBaseName)
{
    st_bool bSuccess = false;

    // get file system pointer from Core lib
    CFileSystem* pFileSystem = CFileSystemInterface::Get( );
    assert(pFileSystem);

    // determine platform-specific shader filenames (no path)
    const CFixedString c_strVertexShaderFilename = CFixedString::Format("%s_vs.%s", strVertexShaderBaseName.c_str( ), TShaderTechniqueClass::GetCompiledShaderExtension( ).c_str( ));
    const CFixedString c_strPixelShaderFilename = CFixedString::Format("%s_ps.%s", strPixelShaderBaseName.c_str( ), TShaderTechniqueClass::GetCompiledShaderExtension( ).c_str( ));

    // search through the paths until first occurance of both files
    st_bool bFilesFound = false;
    CFixedString strVertexFullPath, strPixelFullPath;
    for (st_int32 i = 0; i < st_int32(aSearchPaths.size( )) && !bFilesFound; ++i)
    {
        // look for shader files
        strVertexFullPath = pFileSystem->CleanPlatformFilename(aSearchPaths[i] + c_szFolderSeparator + c_strVertexShaderFilename);
        strPixelFullPath = pFileSystem->CleanPlatformFilename(aSearchPaths[i] + c_szFolderSeparator + c_strPixelShaderFilename);

        if (pFileSystem->FileExists(strVertexFullPath.c_str( )) &&
            pFileSystem->FileExists(strPixelFullPath.c_str( )))
            bFilesFound = true;
    }

    // load pre-compiled shaders (or compile then at run-time is GLSL)
    if (bFilesFound)
    {
        if (m_cTechnique.Load(strVertexFullPath.c_str( ), strPixelFullPath.c_str( ), m_cStateBlock.GetAppState( ), *this))
            bSuccess = true;
        else
            CCore::SetError("Failed to load/compile shader pair [%s & %s]",
                strVertexFullPath.c_str( ), strPixelFullPath.c_str( ));
    }
    else
    {
        CCore::SetError("Failed to locate shader pair [%s & %s] in any of the search paths",
            c_strVertexShaderFilename.c_str( ), c_strPixelShaderFilename.c_str( ));
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::InitFallbackTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitFallbackTextures(void)
{
    st_bool bSuccess = true;

    ++m_nFallbackTextureRefCount;

    if (!m_atFallbackTextures[0].IsValid( ))
    {
        for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
        {
            TTextureClass& tTex = m_atFallbackTextures[i];

            switch (i)
            {
            case TL_DIFFUSE:
            case TL_SPECULAR_MASK:
            case TL_TRANSMISSION_MASK:
                bSuccess &= tTex.LoadColor(0xffffffff); 
                break;
            case TL_NORMAL:
            case TL_DETAIL_NORMAL:
                bSuccess &= tTex.LoadColor(0x7f7fffff);
                break;
            case TL_DETAIL_DIFFUSE:
            case TL_AUX_ATLAS1:
            case TL_AUX_ATLAS2:
                bSuccess &= tTex.LoadColor(0x00000000);
                break;
            }
        }
    }

    return bSuccess;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::ClearLastBoundTextures

CRenderStateRI_TemplateList
inline void CRenderStateRI_t::ClearLastBoundTextures(void)
{
    for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
        m_atLastBoundTextures[i] = TTextureClass( );
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::operator=

CRenderStateRI_TemplateList
inline CRenderStateRI_t& CRenderStateRI_t::operator=(const CRenderStateRI_t& sRight)
{
    return operator=((SRenderState) sRight);
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::operator=
//
//  todo: check that all SRenderState members are accounted for

CRenderStateRI_TemplateList
inline CRenderStateRI_t& CRenderStateRI_t::operator=(const SRenderState& sRight)
{
    // textures
    for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
        m_apTextures[i] = sRight.m_apTextures[i];

    // lighting model
    m_eLightingModel = sRight.m_eLightingModel;

    // ambient
    m_vAmbientColor = sRight.m_vAmbientColor;
    m_eAmbientContrast = sRight.m_eAmbientContrast;
    m_fAmbientContrastFactor = sRight.m_fAmbientContrastFactor;
    m_bAmbientOcclusion = sRight.m_bAmbientOcclusion;

    // diffuse
    m_vDiffuseColor = sRight.m_vDiffuseColor;
    m_fDiffuseScalar = sRight.m_fDiffuseScalar;
    m_bDiffuseAlphaMaskIsOpaque = sRight.m_bDiffuseAlphaMaskIsOpaque;
    m_bBranchLevel1WindWeightsActive = sRight.m_bBranchLevel1WindWeightsActive;

    // detail
    m_eDetailLayer = sRight.m_eDetailLayer;

    // specular
    m_eSpecular = sRight.m_eSpecular;
    m_fShininess = sRight.m_fShininess;
    m_vSpecularColor = sRight.m_vSpecularColor;

    // transmission
    m_eTransmission = sRight.m_eTransmission;
    m_vTransmissionColor = sRight.m_vTransmissionColor;
    m_fTransmissionShadowBrightness = sRight.m_fTransmissionShadowBrightness;
    m_fTransmissionViewDependency = sRight.m_fTransmissionViewDependency;

    // branch seam smoothing
    m_eBranchSeamSmoothing = sRight.m_eBranchSeamSmoothing;
    m_fBranchSeamWeight = sRight.m_fBranchSeamWeight;

    // LOD parameters
    m_eLodMethod = sRight.m_eLodMethod;
    m_bFadeToBillboard = sRight.m_bFadeToBillboard;
    m_bVertBillboard = sRight.m_bVertBillboard;
    m_bHorzBillboard = sRight.m_bHorzBillboard;

    // render states
    m_eShaderGenerationMode = sRight.m_eShaderGenerationMode;
    m_bUsedAsGrass = sRight.m_bUsedAsGrass;
    m_eFaceCulling = sRight.m_eFaceCulling;
    m_bBlending = sRight.m_bBlending;
    m_eAmbientImageLighting = sRight.m_eAmbientImageLighting;
    m_eHueVariation = sRight.m_eHueVariation;

    // fog
    m_eFogCurve = sRight.m_eFogCurve;
    m_eFogColorStyle = sRight.m_eFogColorStyle;

    // shadows
    m_bCastsShadows = sRight.m_bCastsShadows;
    m_bReceivesShadows = sRight.m_bReceivesShadows;
    m_bShadowSmoothing = sRight.m_bShadowSmoothing;
    m_bBranchLevel2WindWeightsActive = sRight.m_bBranchLevel2WindWeightsActive;

    // alpha effects
    m_fAlphaScalar = sRight.m_fAlphaScalar;

    // wind
    m_eWindLod = sRight.m_eWindLod;

    // non-lighting shaders
    m_eRenderPass = sRight.m_eRenderPass;

    // geometry types
    m_bBranchesPresent = sRight.m_bBranchesPresent;
    m_bFrondsPresent = sRight.m_bFrondsPresent;
    m_bLeavesPresent = sRight.m_bLeavesPresent;
    m_bFacingLeavesPresent = sRight.m_bFacingLeavesPresent;
    m_bRigidMeshesPresent = sRight.m_bRigidMeshesPresent;

    // vertex format data
    m_sVertexDecl = sRight.m_sVertexDecl;

    // misc
    m_pDescription = sRight.m_pDescription;
    m_pUserData = sRight.m_pUserData;

    return *this;
}


////////////////////////////////////////////////////////////
//  CRenderStateRI_t::GetHashKey

CRenderStateRI_TemplateList
inline st_int64 CRenderStateRI_t::GetHashKey(void) const
{
    return m_lSortKey;
}

