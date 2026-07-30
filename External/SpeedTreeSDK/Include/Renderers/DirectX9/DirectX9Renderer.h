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
#define VC_EXTRALEAN
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
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
    //  SafeRelease

    #ifndef ST_SAFE_RELEASE
        #define ST_SAFE_RELEASE(p) { if ((p)) { (p)->Release( ); (p) = NULL; } }
    #endif


    ///////////////////////////////////////////////////////////////////////  
    //  Class DX9

    class ST_DLL_LINK DX9
    {
    public:
    static  LPDIRECT3DDEVICE9 ST_CALL_CONV  Device(void);
    static  void              ST_CALL_CONV  SetDevice(LPDIRECT3DDEVICE9 pDevice);

    private:
    static  LPDIRECT3DDEVICE9               m_pDx9;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CStateBlockDirectX9

    class ST_DLL_LINK CStateBlockDirectX9
    {
    public:
                                        CStateBlockDirectX9( );

            st_bool                     Init(const SAppState& sAppState, const SRenderState& sRenderState);
            st_bool                     Bind(void) const;
            void                        ReleaseGfxResources(void);

            // queries
            const SAppState&            GetAppState(void) const;
            const SRenderState&         GetRenderState(void) const;

    private:
            SAppState                   m_sAppState;
            SRenderState                m_sRenderState;

            // alpha-to-coverage
            st_bool                     m_bAlphaToCoverageSupported;
            st_bool                     m_bATI;
    };
    typedef CStateBlockRI<CStateBlockDirectX9> CStateBlock;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CTextureDirectX9

    class ST_DLL_LINK CTextureDirectX9
    {
    public:
                                            CTextureDirectX9( );
    virtual                                 ~CTextureDirectX9( );

            // loading
            st_bool                         Load(const char* pFilename, st_int32 nMaxAnisotropy = 0);
            st_bool                         LoadColor(st_uint32 ulColor);
            st_bool                         LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise);
            st_bool                         LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
            st_bool                         ReleaseGfxResources(void);
            st_bool                         IsValid(void) const;
            const STextureInfo&             GetInfo(void) const;

            // render
            LPDIRECT3DTEXTURE9              GetTextureObject(void) const;
    static  void               ST_CALL_CONV SetSamplerStates(void);
    static  void               ST_CALL_CONV CreateSharedSamplers(st_int32 nMaxAnisotropy);

            // other
            CTextureDirectX9&               operator=(const CTextureDirectX9& cRight);
            st_bool                         operator!=(const CTextureDirectX9& cRight) const;

    private:
            LPDIRECT3DTEXTURE9              m_pTexture;
            STextureInfo                    m_sInfo;
    static  st_int32                        m_nMaxAnisotropy;
    };
    typedef CTextureRI<CTextureDirectX9> CTexture;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CRenderTargetDirectX9

    class ST_DLL_LINK CRenderTargetDirectX9
    {
    public:
                                        CRenderTargetDirectX9( );
                                        ~CRenderTargetDirectX9( );

            st_bool                     InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
            void                        ReleaseGfxResources(void);

            void                        Clear(const Vec4& vColor);

            st_bool                     SetAsTarget(void);                     
            void                        ReleaseAsTarget(void);                        
            st_bool                     BindAsTexture(st_int32 nRegister, st_bool bPointFilter) const;
            void                        UnBindAsTexture(st_int32 nRegister) const;

            void                        OnResetDevice(void);
            void                        OnLostDevice(void);

    static  st_bool                     SetGroupAsTarget(const CRenderTargetDirectX9** pTargets, st_int32 nNumTargets);
    static  void                        ReleaseGroupAsTarget(const CRenderTargetDirectX9** pTargets, st_int32 nNumTargets);

    private:
            LPDIRECT3DTEXTURE9          m_pColorTexture;
            LPDIRECT3DSURFACE9          m_pColorSurface;
            LPDIRECT3DTEXTURE9          m_pDepthStencilTexture;
            LPDIRECT3DSURFACE9          m_pDepthStencilSurface;

            // used to save and restore the current view and viewport
    static  LPDIRECT3DSURFACE9          m_pPreviouslyActiveRenderTargetSurface;
    static  LPDIRECT3DSURFACE9          m_pPreviouslyActiveDepthStencilSurface;
    static  D3DVIEWPORT9                m_sPreviouslyActiveViewport;

            ERenderTargetType           m_eType;
            st_int32                    m_nWidth;
            st_int32                    m_nHeight;
    };
    typedef CRenderTargetRI<CRenderTargetDirectX9> CRenderTarget;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CShaderConstantBufferDirectX9

    class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantBufferDirectX9
    {
    public:
                                        CShaderConstantBufferDirectX9( );
                                        ~CShaderConstantBufferDirectX9( );

            st_bool        ST_CALL_CONV Init(size_t siSizeOfDerivedClass, st_int32 nRegister);
            void           ST_CALL_CONV ReleaseGfxResources(void);

            st_bool        ST_CALL_CONV Update(void) const;
            st_bool        ST_CALL_CONV Bind(void) const;

            // textures
    static  st_bool        ST_CALL_CONV SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately = true);
    static  void           ST_CALL_CONV SubmitSetTexturesInBatch(void);

    private:
            st_byte*                    m_pBuffer;
            size_t                      m_siSizeOfDerivedClass;
            st_int32                    m_nRegister;
    };
    typedef CShaderConstantBufferRI<CShaderConstantBufferDirectX9, CTexture> CShaderConstantBuffer;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CShaderTechniqueDirectX9

    class ST_DLL_LINK CShaderTechniqueDirectX9
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
                    void                    ReleaseGfxResources(void);

                    LPDIRECT3DVERTEXSHADER9 m_pDx9Shader;
            };


            ///////////////////////////////////////////////////////////////////////  
            //  Class CPixelShader

            class ST_DLL_LINK CPixelShader
            {
            public:
                                            CPixelShader( );

                    st_bool                 Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
                    st_bool                 IsValid(void) const;
                    void                    ReleaseGfxResources(void);

                    LPDIRECT3DPIXELSHADER9  m_pDx9Shader;
            };

            st_bool                         Link(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader);
            st_bool                         Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader, st_bool bDepthOnlyOpaqueMode = false) const;
    static  void               ST_CALL_CONV UnBind(void);
            st_bool                         ReleaseGfxResources(void);

            st_bool                         LoadProgramBinary(const char*, const char*) { return false; }
            st_bool                         SaveProgramBinary(const char*, const char*) { return false; }

    static  CFixedString       ST_CALL_CONV GetCompiledShaderExtension(void);
    static  CFixedString       ST_CALL_CONV GetCompiledShaderFolder(void);
    static  st_bool            ST_CALL_CONV VertexDeclNeedsInstancingAttribs(void);
    };
    typedef CShaderTechniqueRI<CShaderTechniqueDirectX9> CShaderTechnique;
    

    ///////////////////////////////////////////////////////////////////////  
    //  Class CGeometryBufferDirectX9

    class ST_DLL_LINK CGeometryBufferDirectX9
    {
    public:
            friend class CInstancingMgrDirectX9;

                                            CGeometryBufferDirectX9( );
    virtual                                 ~CGeometryBufferDirectX9( );

            // vertex buffer
            st_bool                         SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl);
            st_bool                         CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize);
            st_bool                         OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream = 0);

            // direct vertex buffer write
            void*                           LockForWrite(st_uint32 uiBufferSize); // will resize on-the-fly if necessary
            st_bool                         UnlockFromWrite(void) const;

            st_bool                         VertexBufferIsValid(void) const;

            st_bool                         EnableFormat(void) const;
    static  st_bool            ST_CALL_CONV DisableFormat(void);
            st_bool                         BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize, st_uint32 uiOffsetInBytes = 0) const;
    static  st_bool            ST_CALL_CONV UnBindVertexBuffer(st_uint32 uiStream);

            // mesh instancing support
    static  st_bool            ST_CALL_CONV SetStreamFrequency(st_uint32 uiStream, st_uint32 uiFreq);

            // index buffer
            st_bool                         SetIndexFormat(EIndexFormat eFormat);
            st_bool                         CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices);
            st_bool                         OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset);
            st_bool                         IndexBufferIsValid(void) const;
            st_uint32                       IndexSize(void) const;
            st_bool                         ClearIndices(void);

            st_bool                         BindIndexBuffer(void) const;
    static  st_bool            ST_CALL_CONV UnBindIndexBuffer(void);

            void                            ReleaseGfxResources(void);

            // render functions
            st_bool                         RenderIndexed(EPrimitiveType ePrimType, 
                                                          st_uint32 uiStartIndex, 
                                                          st_uint32 uiNumIndices, 
                                                          st_uint32 uiMinIndex = 0,
                                                          st_uint32 uiNumVertices = 0) const;
            st_bool                         RenderArrays(EPrimitiveType ePrimType, 
                                                         st_uint32 uiStartVertex, 
                                                         st_uint32 uiNumVertices) const;

    private:
            st_bool                         IsFormatSet(void) const;

            LPDIRECT3DVERTEXDECLARATION9    m_pVertexDeclaration;
            LPDIRECT3DVERTEXBUFFER9         m_pVertexBuffer;
            LPDIRECT3DINDEXBUFFER9          m_pIndexBuffer;
            EIndexFormat                    m_eIndexFormat;
            st_uint32                       m_uiCurrentVertexBufferSize;    // in bytes
            st_uint32                       m_uiCurrentIndexBufferSize;     // in bytes
    };
    typedef CGeometryBufferRI<CGeometryBufferDirectX9, CShaderTechnique> CGeometryBuffer;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CInstancingMgrDirectX9
    //
    //  One CInstancingMgrDirectX9 object is used per base tree.

    class ST_DLL_LINK CInstancingMgrDirectX9
    {
    public:
                                            CInstancingMgrDirectX9( );
    virtual                                 ~CInstancingMgrDirectX9( );

            // functions called by CInstancingMgrRI in the RenderInterface library
            st_bool                         Init(SVertexDecl::EInstanceType eInstanceType, 
                                                 st_int32 nNumLods, 
                                                 const CGeometryBuffer* pGeometryBuffers, 
                                                 st_int32 nNumGeometryBuffers);
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
                CGeometryBuffer             m_cInstanceBuffer;
                st_bool                     m_bBufferLocked;
            };
            CArray<SInstancesPerLod>        m_aInstances;       // one element per LOD level in the base tree

            CArray<CGeometryBuffer>         m_aCompositeVertexFormats;
            const CGeometryBuffer*          m_pObjectBuffers;
            size_t                          m_siSizeOfPerInstanceData;
    };
    typedef CInstancingMgrRI<CInstancingMgrDirectX9, CGeometryBuffer> CInstancingMgr;


    // include inline functions
    #include "RenderInterface/TemplateTypedefs.h"
    #include "Renderers/DirectX9/Texture_inl.h"
    #include "Renderers/DirectX9/Shaders_inl.h"
    #include "Renderers/DirectX9/GeometryBuffer_inl.h"
    #include "Renderers/DirectX9/InstancingManager_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(pop)
#endif
