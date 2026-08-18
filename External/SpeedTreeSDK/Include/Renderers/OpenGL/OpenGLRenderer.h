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
//
//  *** Release version 7.0.0 ***


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#pragma once
#ifndef ST_OPENGL
    #define ST_OPENGL
#endif
#define VC_EXTRALEAN
#define GLEW_STATIC
#define GLEW_NO_GLU
#include "Utilities/glew.h"
#ifdef _WIN32
    #include "Utilities/wglew.h"
#endif
#include "Core/ExportBegin.h"
#include "Core/Mutex.h"
#include "RenderInterface/ForestRI.h"
#include "RenderInterface/TerrainRI.h"
#include "RenderInterface/SkyRI.h"


///////////////////////////////////////////////////////////////////////  
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(push, 4)
#endif


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    ///////////////////////////////////////////////////////////////////////  
    //  Constants

    const st_int32 c_nLinkedProgramCacheInitialReserve = 40;


    ///////////////////////////////////////////////////////////////////////
    //  Class CGraphicsCapsOpenGL

    class ST_DLL_LINK CGraphicsCapsOpenGL
    {
    public:
    static  GLenum         ST_CALL_CONV InitGlew(void);
    static  st_bool        ST_CALL_CONV IsInstancingSupported(void);
    static  st_bool        ST_CALL_CONV IsGlsl150OrHigher(void);

    private:
    static  st_bool                     m_bGlewInitialized;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CStateBlockOpenGL

    class ST_DLL_LINK CStateBlockOpenGL
    {
    public:
                                        CStateBlockOpenGL( );

            st_bool                     Init(const SAppState& sAppState, const SRenderState& sRenderState);
            st_bool                     Bind(void) const;
            void                        ReleaseGfxResources(void);

    private:
            SAppState                   m_sAppState;
            SRenderState                m_sRenderState;
            st_bool                     m_bAlphaToCoverageSupported;
            GLuint                      m_uiDisplayList;
    };
    typedef CStateBlockRI<CStateBlockOpenGL> CStateBlock;


    ///////////////////////////////////////////////////////////////////////
    //  Class CTextureOpenGL

    class ST_DLL_LINK CTextureOpenGL
    {
    public:
                                    CTextureOpenGL( );
    virtual                         ~CTextureOpenGL( );

            // loading
            st_bool                 Load(const char* pFilename, st_int32 nMaxAnisotropy = 0);
            st_bool                 LoadColor(st_uint32 uiColor); // 0xAABBGGRR
            st_bool                 LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise);
            st_bool                 LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
            st_bool                 ReleaseGfxResources(void);
            st_bool                 IsValid(void) const;
            const STextureInfo&     GetInfo(void) const;

            // render
            st_uint32               GetTextureObject(void) const;
    static  void       ST_CALL_CONV SetSamplerStates(void);
    static  void       ST_CALL_CONV CreateSharedSamplers(st_int32 nMaxAnisotropy);

            // other
            CTextureOpenGL&         operator=(const CTextureOpenGL& cRight);
            st_bool                 operator!=(const CTextureOpenGL& cRight) const;

    private:
            st_bool                 LoadTGA(const CFixedString& strFilename, st_int32 nMaxAnisotropy = 0);
            st_bool                 LoadDDS(const CFixedString& strFilename, st_int32 nMaxAnisotropy = 0);

            GLuint                  m_uiTexture;
            STextureInfo            m_sInfo;

    };
    typedef CTextureRI<CTextureOpenGL> CTexture;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CRenderTargetOpenGL

    class ST_DLL_LINK_STATIC_MEMVAR CRenderTargetOpenGL
    {
    public:
                                        CRenderTargetOpenGL( );
                                        ~CRenderTargetOpenGL( );

            st_bool                     InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
            void                        ReleaseGfxResources(void);

            void                        Clear(const Vec4& vColor);

            st_bool                     SetAsTarget(void);                     
            void                        ReleaseAsTarget(void);                        
            st_bool                     BindAsTexture(st_int32 nRegister, st_bool bPointFilter) const;
            void                        UnBindAsTexture(st_int32 nRegister) const;

            GLuint                      GetFbo(void) const;

            void                        OnResetDevice(void);
            void                        OnLostDevice(void);

    static  st_bool                     SetGroupAsTarget(const CRenderTargetOpenGL** pTargets, st_int32 nNumTargets);
    static  void                        ReleaseGroupAsTarget(const CRenderTargetOpenGL** pTargets, st_int32 nNumTargets);

    private:
            ERenderTargetType           m_eType;
            st_int32                    m_nWidth;
            st_int32                    m_nHeight;
            st_int32                    m_nNumSamples;
            GLint                       m_aBackupViewport[4];

            GLuint                      m_uiFbo;
            GLuint                      m_uiRenderBuffer;
            GLuint                      m_uiRenderTexture;
    };
    typedef CRenderTargetRI<CRenderTargetOpenGL> CRenderTarget;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CShaderConstantBufferOpenGL

    class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantBufferOpenGL
    {
    public:
                                        CShaderConstantBufferOpenGL( );
                                        ~CShaderConstantBufferOpenGL( );

            st_bool        ST_CALL_CONV Init(size_t siSizeOfDerivedClass, st_int32 nRegister);
            void           ST_CALL_CONV ReleaseGfxResources(void);

            st_bool        ST_CALL_CONV Update(void) const;
            st_bool        ST_CALL_CONV Bind(void) const;

            // textures
    static  st_bool        ST_CALL_CONV SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately = true);
    static  void           ST_CALL_CONV SubmitSetTexturesInBatch(void);

    private:
            GLuint                      m_uiBuffer;
            size_t                      m_siSizeOfDerivedClass;
            GLuint                      m_uiRegister;
    };
    typedef CShaderConstantBufferRI<CShaderConstantBufferOpenGL, CTexture> CShaderConstantBuffer;


    ///////////////////////////////////////////////////////////////////////
    //  Class CShaderTechniqueOpenGL

    class ST_DLL_LINK_STATIC_MEMVAR CShaderTechniqueOpenGL
    {
    public:

            ///////////////////////////////////////////////////////////////////////  
            //  Class CVertexShader

            class ST_DLL_LINK CVertexShader
            {
            public:
                                            CVertexShader( );

                    st_bool                 Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
                    st_bool                 IsValid(void) const;
                    st_bool                 ReleaseGfxResources(void);

                    GLuint                  m_uiShader;
            };


            ///////////////////////////////////////////////////////////////////////  
            //  Class CPixelShader

            class ST_DLL_LINK CPixelShader
            {
            public:
                                            CPixelShader( );

                    st_bool                 Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
                    st_bool                 IsValid(void) const;
                    st_bool                 ReleaseGfxResources(void);

                    GLuint                  m_uiShader;
            };

                                            CShaderTechniqueOpenGL( );

            st_bool                         Link(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader);
            void                            PrepareProgram(void);
            st_bool                         Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const;
    static  void               ST_CALL_CONV UnBind(void);
            st_bool                         ReleaseGfxResources(void);
        
            st_bool                         LoadProgramBinary(const char* pVertexFilename, const char* pPixelFilename);
            st_bool                         SaveProgramBinary(const char* pVertexFilename, const char* pPixelFilename);

    static  st_bool                         CommitSingleInstance(void);
    static  st_bool                         CommitInstanceList(void);

    static  CFixedString       ST_CALL_CONV GetCompiledShaderExtension(void);
    static  CFixedString       ST_CALL_CONV GetCompiledShaderFolder(void);
    static  st_bool            ST_CALL_CONV VertexDeclNeedsInstancingAttribs(void);

    private:
            // map vertex+pixel shaders to a linked program object
            typedef CResourceCache<CFixedString, GLuint> CLinkedProgramCache;

            GLuint                          m_uiProgram; // vs & ps linked together
            st_char                         m_szLinkCacheFilename[c_siFixedStringDefaultLength];
    static  CLinkedProgramCache*            m_pLinkedProgramCache;
    };
    typedef CShaderTechniqueRI<CShaderTechniqueOpenGL> CShaderTechnique;


    ///////////////////////////////////////////////////////////////////////
    //  Class CGeometryBufferOpenGL

    class ST_DLL_LINK CGeometryBufferOpenGL
    {
    public:
                                    CGeometryBufferOpenGL( );
    virtual                         ~CGeometryBufferOpenGL( );

            // vertex buffer
            st_bool                 SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl);
            st_bool                 CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize);
            st_bool                 OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream);
            st_bool                 VertexBufferIsValid(void) const;

            st_bool                 EnableFormat(void) const;
    static  st_bool    ST_CALL_CONV DisableFormat(void);
            st_bool                 BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const;
    static  st_bool    ST_CALL_CONV UnBindVertexBuffer(st_uint32 uiStream);

            // index buffer
            st_bool                 SetIndexFormat(EIndexFormat eFormat);
            st_bool                 CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices);
            st_bool                 OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset);
            st_bool                 IndexBufferIsValid(void) const;
            st_uint32               IndexSize(void) const;
            st_bool                 ClearIndices(void);

            st_bool                 BindIndexBuffer(void) const;
    static  st_bool    ST_CALL_CONV UnBindIndexBuffer(void);

            void                    ReleaseGfxResources(void);

            // render functions
            st_bool                 RenderIndexed(EPrimitiveType ePrimType,
                                                  st_uint32 uiStartIndex,
                                                  st_uint32 uiNumIndices,
                                                  st_uint32 uiMinIndex = 0,
                                                  st_uint32 uiNumVertices = 0) const;
            st_bool                 RenderIndexedInstanced(EPrimitiveType ePrimType,
                                                           st_uint32 uiStartIndex,
                                                           st_uint32 uiNumIndices,
                                                           st_uint32 uiNumInstances,
                                                           st_uint32 uiStartInstanceLocation = 0) const;
            st_bool                 RenderArrays(EPrimitiveType ePrimType,
                                                 st_uint32 uiStartVertex,
                                                 st_uint32 uiNumVertices) const;

            // specific to OpenGL instancing renderer
            GLuint                  CreateInstancingVAO(const CGeometryBufferRI<CGeometryBufferOpenGL, CShaderTechnique>* pBuffer, GLuint uiInstanceBuffer) const;

    private:
            struct ST_DLL_LINK SAttribParams
            {
                                    SAttribParams( );

                st_bool             IsActive(void) const;

                st_int32            m_nOffset;
                st_int32            m_nNumComponents;
                GLenum              m_eDataType;
            };

            st_bool                 IsVertexFormatSet(void) const;

            GLuint                  m_uiVertexBuffer;
            GLuint                  m_uiIndexBuffer;
            EIndexFormat            m_eIndexFormat;
            SAttribParams           m_asAttribParams[VERTEX_ATTRIB_COUNT];
    };
    typedef CGeometryBufferRI<CGeometryBufferOpenGL, CShaderTechnique> CGeometryBuffer;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CInstancingMgrOpenGL
    //
    //  One CInstancingMgrOpenGL object is used per base tree.

    class ST_DLL_LINK CInstancingMgrOpenGL
    {
    public:
                                            CInstancingMgrOpenGL( );
    virtual                                 ~CInstancingMgrOpenGL( );

            // functions called by CInstancingMgrRI in the RenderInterface library
            st_bool                         Init(SVertexDecl::EInstanceType eInstanceType, st_int32 nNumLods, const CGeometryBuffer* pGeometryBuffers, st_int32 nNumGeometryBuffers);
            void                            ReleaseGfxResources(void);

            st_bool                         Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances);
            st_bool                         Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const;

            void*                           LockInstanceBufferForWrite(st_int32 nLod, st_int32 nMaxNumInstances);
            st_bool                         UnlockInstanceBufferFromWrite(st_int32 nLod, st_int32 nActualNumInstances);

            // queries
            st_int32                        NumInstances(st_int32 nLod) const;

    private:
            struct SInstancesPerLod
            {
                                            SInstancesPerLod( );

                st_int32                    m_nNumInstances;
                GLuint                      m_uiInstanceBuffer;
                size_t                      m_siInstanceBufferSize;
                st_bool                     m_bBufferLocked;
            };
            CArray<SInstancesPerLod>        m_asInstances;      // one element per LOD level in the base tree

            const CGeometryBuffer*          m_pObjectBuffers;
    mutable CArray<GLuint>                  m_aVAOs;            // size needs to match # of geometry buffers passed to Init()
            size_t                          m_siSizeOfPerInstanceData;
            st_bool                         m_bMeshInstancingSupported;
    };
    typedef CInstancingMgrRI<CInstancingMgrOpenGL, CGeometryBuffer> CInstancingMgr;

    // include inline code
    #include "RenderInterface/TemplateTypedefs.h"
    #include "Renderers/OpenGL/Texture_inl.h"
    #include "Renderers/OpenGL/Shaders_inl.h"
    #include "Renderers/OpenGL/GeometryBuffer_inl.h"
    #include "Renderers/OpenGL/InstancingManager_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
