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
//  *** Release version 7.1.0 ***


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#define ST_RENDER_STATS
#include "RenderInterface/GraphicsApiAbstractionRI.h"
#include "RenderInterface/ShaderConstantBuffers.h"
#include "Core/Timer.h"


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
    //  Enumeration EStatsCategory

    enum EStatsCategory
    {
        // main geometry types
        STATS_CATEGORY_3D_TREES,
        STATS_CATEGORY_BILLBOARDS,
        STATS_CATEGORY_TERRAIN,
        STATS_CATEGORY_SKY,

        // utility
        STATS_CATEGORY_COUNT
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CRenderStats

    class ST_DLL_LINK CRenderStats
    {
    public:
            template <class T>
            class ST_DLL_LINK CStatPrimitive
            {
            public:
                                    CStatPrimitive( );
                                    CStatPrimitive(const T& tRight);

                    void            Zero(void);

                    T               operator +(const T& tRight) const;
                    T               operator +=(const T& tRight);
                                    operator T(void) const;

            private:
                    T               m_tData;
            };

            typedef CStatPrimitive<st_int32>    CStatInt32;
            typedef CStatPrimitive<st_int64>    CStatInt64;
            typedef CStatPrimitive<st_float32>  CStatFloat32;

            struct ST_DLL_LINK SObjectStats
            {
            public:
                    friend class CRenderStats;

                                        SObjectStats( );

                    void                Reset(void);

                    CFixedString        m_strName;

                    // render stats
                    CStatInt32          m_nNumInstances;
                    CStatInt32          m_nNumDrawCalls;
                    CStatInt32          m_nNumTextureBinds;
                    CStatInt32          m_nNumShaderBinds;
                    CStatInt32          m_nNumTrianglesRendered;
            };

                                        CRenderStats( );
                                        ~CRenderStats( );

            void                        Reset(void);
            SObjectStats&               GetObjectStats(const CFixedString& strName);
            const CArray<SObjectStats>& GetObjectsArray(void) const;

            CStatInt32                  m_nFrameCount;
            CStatFloat32                m_fSpeedTreeCullAndUpdateTime;
            CStatFloat32                m_fOtherCullAndUpdateTime;
            CStatFloat32                m_fOverallCullAndUpdateTime;

    private:
            CArray<SObjectStats>        m_asObjects;
            #ifndef ST_RENDER_STATS
                SObjectStats            m_sStub;
            #endif
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Enumeration ETextureAlphaRenderMode

    enum ETextureAlphaRenderMode
    {
        TRANS_TEXTURE_ALPHA_TESTING,
        TRANS_TEXTURE_ALPHA_TO_COVERAGE,
        TRANS_TEXTURE_BLENDING,
        TRANS_TEXTURE_NOTHING,
        TRANS_TEXTURE_UNASSIGNED
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SForestRenderInfo

    struct ST_DLL_LINK SForestRenderInfo
    {
                                    SForestRenderInfo( );

        // app-level
        SAppState                   m_sAppState;

        // general rendering
        st_int32                    m_nMaxAnisotropy;               // see note below
        st_bool                     m_bHorizontalBillboards;        // see note below
        st_bool                     m_bDepthOnlyPrepass;            // see note below
        st_float32                  m_fNearClip;
        st_float32                  m_fFarClip;
        st_bool                     m_bTexturingEnabled;
        st_float32                  m_fTextureAlphaScalar3d;
        st_float32                  m_fTextureAlphaScalarGrass;
        st_float32                  m_fTextureAlphaScalarBillboards;

        // lighting
        SSimpleMaterial             m_sLightMaterial;
        CFixedString                m_strImageBasedAmbientLightingFilename;

        // fog
        st_float32                  m_fFogStartDistance;
        st_float32                  m_fFogEndDistance;
        st_float32                  m_fFogDensity;
        Vec3                        m_vFogColor;

        // sky
        Vec3                        m_vSkyColor;
        st_float32                  m_fSkyFogMin;
        st_float32                  m_fSkyFogMax;

        // sun
        Vec3                        m_vSunColor;
        st_float32                  m_fSunSize;
        st_float32                  m_fSunSpreadExponent;

        // shadows
        st_bool                     m_bShadowsEnabled;
        st_int32                    m_nShadowsNumMaps;
        st_int32                    m_nShadowsResolution;
        st_float32                  m_afShadowMapRanges[c_nMaxNumShadowMaps];
        st_float32                  m_fShadowFadePercent;

        // *note: these values will be ignored if changed after CForestRI::InitGfx() has been called
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Enumeration ETextureBindMode

    enum ETextureBindMode
    {
        TEXTURE_BIND_ENABLED,       // textures are bound as loaded in the render state object
        TEXTURE_BIND_DISABLED,      // no textures are bound
        TEXTURE_BIND_FALLBACK       // fallback textures are bound (used for "untextured" render mode)
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CRenderStateRI

    #define CRenderStateRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TShaderTechniqueClass, class TShaderConstantBufferClass>

    CRenderStateRI_TemplateList
    class ST_DLL_LINK CRenderStateRI : public SRenderState
    {
    public:

                                            CRenderStateRI( );
    virtual                                 ~CRenderStateRI( );

            // creation/destruction
            st_bool                         InitGfx(const SAppState& sAppState,
                                                    const CArray<CFixedString>& aSearchPaths, 
                                                    st_int32 nMaxAnisotropy,
                                                    st_float32 fTextureAlphaScalar,
                                                    const CFixedString& strVertexShaderBaseName,
                                                    const CFixedString& strPixelShaderBaseName,
                                                    const CWind* pWind);
            void                            ReleaseGfxResources(void);
            const TShaderTechniqueClass&    GetTechnique(void) const;
            const TStateBlockClass&         GetStateBlock(void) const;

            // render loop
            st_bool                         BindConstantBuffer(void) const;
            st_bool                         BindShader(void) const;
            st_bool                         BindTextures(ERenderPass ePass, ETextureBindMode eTextureBindMode) const;
            st_bool                         BindStateBlock(void) const;
            st_bool                         BindMaterialWhole(ERenderPass ePass, ETextureBindMode eTextureBindMode) const;
    static  st_bool    ST_CALL_CONV         UnBind(void);
    static  void       ST_CALL_CONV         ClearLastBoundTextures(void);

            // utility
            CRenderStateRI&                 operator=(const CRenderStateRI& sRight);
            CRenderStateRI&                 operator=(const SRenderState& sRight);
            st_int64                        GetHashKey(void) const;

    private:
            st_bool                         LoadTextures(const CArray<CFixedString>& aSearchPaths, st_int32 nMaxAnisotropy);
            st_bool                         InitTexture(const char* pFilename,
                                                        TTextureClass& tTextureObject,
                                                        const CArray<CFixedString>& aSearchPaths, 
                                                        st_int32 nMaxAnisotropy);
            st_bool                         InitConstantBuffer(const SAppState& sAppState, st_float32 fTextureAlphaScalar, const CWind* pWind);
            st_bool                         BindTexture(st_int32 nLayer, const TTextureClass& tTexture) const;
            st_bool                         LoadShaders(const CArray<CFixedString>& aShaderSearchPaths, 
                                                        const CFixedString& strVertexShaderBaseName,
                                                        const CFixedString& strPixelShaderBaseName);
    static  st_bool    ST_CALL_CONV         InitFallbackTextures(void);

            TShaderTechniqueClass           m_cTechnique;                               // houses vertex & pixel shaders
            TStateBlockClass                m_cStateBlock;                              // houses depth, rasterizer, and blend state
            SMaterialConstBuf_t             m_sConstantBuffer;
            st_int64                        m_lSortKey;

            TTextureClass                   m_atTextureObjects[TL_NUM_TEX_LAYERS];      // this render state's texture bank
            st_bool                         m_bFallbackTexturesInited;
    static  TTextureClass                   m_atLastBoundTextures[TL_NUM_TEX_LAYERS];   // used to prevent redundant texture binds
    static  TTextureClass                   m_atFallbackTextures[TL_NUM_TEX_LAYERS];    // small, simple textures used as defaults
    static  st_int32                        m_nFallbackTextureRefCount;
    };
    #define CRenderStateRI_t CRenderStateRI<TStateBlockClass, TTextureClass, TShaderTechniqueClass, TShaderConstantBufferClass>


    ///////////////////////////////////////////////////////////////////////  
    //  Class CTreeRI

    #define CTreeRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TShaderTechniqueClass, class TShaderConstantBufferClass>

    CTreeRI_TemplateList
    class ST_DLL_LINK CTreeRI : public CTree
    {
    public:
                                                        CTreeRI( );
            virtual                                     ~CTreeRI( );
    
            // graphics
            st_bool                                     InitGfx(const SAppState& sAppState,
                                                                const CArray<CFixedString>& aSearchPaths,
                                                                st_int32 nMaxAnisotropy = 0,
                                                                st_float32 fTextureAlphaScalar = 1.0f);
            void                                        ReleaseGfxResources(void);
            st_bool                                     GraphicsAreInitialized(void) const;

            // 3d geometry buffers
            const TGeometryBufferClass*                 GetGeometryBuffer(st_int32 nLod, st_int32 nDrawCall) const;
            st_int32                                    GetGeometryBufferOffset(st_int32 nLod, st_int32 nDrawCall) const;
            const CArray<TGeometryBufferClass>&         GetGeometryBuffers(void) const;
            void                                        GetVertexBufferUsage(st_int32& nVertexBuffers, st_int32& nIndexBuffers) const;

            // render states
            st_bool                                     BindConstantBuffers(void) const;
            st_bool                                     UpdateWindConstantBuffer(void) const;
            const CArray<CRenderStateRI_t>&             Get3dRenderStates(ERenderPass eShaderType) const;
            const CRenderStateRI_t&                     GetBillboardRenderState(ERenderPass eShaderType) const;

            // billboards
            const TGeometryBufferClass&                 GetBillboardGeometryBuffer(void) const;

    protected:
            st_int32                                    FindMaxNumDrawCallsPerLod(void);

            st_bool                                     InitRenderStates(const SAppState& sAppState, 
                                                                         const CArray<CFixedString>& aSearchPaths, 
                                                                         st_int32 nMaxAnisotropy,
                                                                         st_float32 fTextureAlphaScalar);
            st_bool                                     Init3dGeometry(void);
            st_bool                                     InitBillboardGeometry(void);
            st_bool                                     InitConstantBuffers(void);

            st_bool                                     InitTexture(const char* pFilename,
                                                                    TTextureClass& texTextureObject,
                                                                    const CArray<CFixedString>& aSearchPaths, 
                                                                    st_int32 nMaxAnisotropy);

            // materials/textures
            CArray<CRenderStateRI_t>                    m_aa3dRenderStates[RENDER_PASS_COUNT];
            CRenderStateRI_t                            m_aBillboardRenderStates[RENDER_PASS_COUNT];

            // geometry
            CArray<TGeometryBufferClass>                m_atGeometryBuffers;
            st_int32                                    m_nMaxNumDrawCallsPerLod;

            // constant buffers
            SBaseTreeConstBuf_t                         m_sBaseTreeConstantBuffer;
            SWindDynamicsConstBuf_t                     m_sWindDynamicsConstantBuffer;

            // billboards
            TGeometryBufferClass                        m_tBillboardGeometryBuffer;

            // textures
            CArray<st_float32>                          m_aModifiedBillboardTexCoords;      // copy of CTree::SGeometry::m_sVertBBs, but modified to include shader hints
            const CArray<CFixedString>*                 m_pSearchPaths;

            // misc
            st_bool                                     m_bGraphicsInitialized;

    private:
                                                        CTreeRI(const CTreeRI& cRight);     // copying CTreeRI disabled
    };
    #define CTreeRI_t CTreeRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TShaderTechniqueClass, TShaderConstantBufferClass>


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SInstancedDrawStats

    struct ST_DLL_LINK SInstancedDrawStats
    {
                                        SInstancedDrawStats( );

            st_int32                    m_nNumInstancesDrawn;
            st_int32                    m_nNumDrawCalls;
            st_int32                    m_nBatchSize;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CInstancingMgrRI

    #define CInstancingMgrRI_TemplateList template<class TInstanceMgrPolicy, class TGeometryBufferClass>

    #ifdef __CELLOS_LV2__
        const st_int32 c_nNumInstBuffers = 3;
    #else
        const st_int32 c_nNumInstBuffers = 2;
    #endif

    CInstancingMgrRI_TemplateList
    class ST_DLL_LINK CInstancingMgrRI
    {
    public:

                                        CInstancingMgrRI( );
            virtual                     ~CInstancingMgrRI( );

            // init
            st_bool                     Init3dTrees(st_int32 nNumLods, const CArray<TGeometryBufferClass>& aGeometryBuffers);
            st_bool                     InitGrass(const CArray<TGeometryBufferClass>& aGeometryBuffers);
            st_bool                     InitBillboards(const TGeometryBufferClass* pGeometryBuffer);
            void                        ReleaseGfxResources(void);

            // updates
            st_bool                     Update3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods);
            st_bool                     UpdateGrassInstanceBuffers(const TRowColCellPtrMap& mCells);

            // billboard update
            st_int32                    CopyBillboardInstancesToGpu(const CTree* pBaseTree, const TRowColCellPtrMap& mCells); // return # bbs, -1 on error

            // render
            st_bool                     Render3dTrees(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const;
            st_bool                     RenderGrass(st_int32 nGeometryBufferIndex, SInstancedDrawStats& sStats) const;
            st_bool                     RenderBillboards(SInstancedDrawStats& sStats) const;

            // simple queries
            st_bool                     IsInitialized(void) const;
            st_int32                    NumInstances(st_int32 nLod = 0) const;

    private:
            void                        AdvanceMgrIndex(void);

            TInstanceMgrPolicy          m_atInstanceMgrPolicies[c_nNumInstBuffers];
            st_int32                    m_nActiveMgrIndex;
            st_bool                     m_bInitialized;
    };
    #define CInstancingMgrRI_t CInstancingMgrRI<TInstanceMgrPolicy, TGeometryBufferClass>


    ///////////////////////////////////////////////////////////////////////  
    //  Class CVisibleInstancesRI

    #define CVisibleInstancesRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TInstancingMgrClass, class TShaderTechniqueClass, class TShaderConstantBufferClass>

    CVisibleInstancesRI_TemplateList
    class ST_DLL_LINK CVisibleInstancesRI : public CVisibleInstances
    {
    public:
            // SForestInstancingData stores instance and billboard instancing 
            // information for a single base tree
            struct SForestInstancingData
            {
                                            SForestInstancingData( );

                const CTree*                m_pBaseTree;

                // mesh instancing support
                TInstancingMgrClass         m_t3dTreeInstancingMgr;
                TInstancingMgrClass         m_tBillboardInstancingMgr;
            };
            typedef CMap<const CTree*, SForestInstancingData*> TInstanceDataPtrMap;

                                            CVisibleInstancesRI(EPopulationType ePopulationType = POPULATION_TREES, st_bool bTrackNearestInsts = false);
                                            ~CVisibleInstancesRI( );

            void                            SetHeapReserves(const SHeapReserves& sHeapReserves);
            st_bool                         InitGfx(const CArray<CTreeRI_t*>& aBaseTrees);
            void                            Clear(void);
            void                            ReleaseGfxResources(void);

            // update
            st_int32                        Cull3dTreesAndUpdateInstanceBuffers(const CView& cView);
            st_bool                         UpdateGrassInstanceBuffers(const CTreeRI_t* pBaseGrass);

            // billboard update
            st_int32                        CopyBillboardInstancesToGpu(const CTreeRI_t* pBaseTree);

            void                            NotifyOfPopulationChange(void);

            const SForestInstancingData*    FindInstancingDataByBaseTree(const CTree* pTree) const;
            SForestInstancingData*          FindOrAddInstancingDataByBaseTree(const CTree* pTree);
            const TInstanceDataPtrMap&      GetPerBaseInstancingDataMap(void) const;

    private:
            TInstanceDataPtrMap             m_mPerBaseInstancingDataMap;
    };
    #define CVisibleInstancesRI_t CVisibleInstancesRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TInstancingMgrClass, TShaderTechniqueClass, TShaderConstantBufferClass>


    ///////////////////////////////////////////////////////////////////////  
    //  Class CForestRI
    
    #define CForestRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TInstancingMgrClass, class TShaderTechniqueClass, class TShaderConstantBufferClass>

    CForestRI_TemplateList
    class ST_DLL_LINK CForestRI : public CForest
    {
    public:
                                            CForestRI( );
                                            ~CForestRI( );
    
            // general graphics
            void                            ReleaseGfxResources(void);
            void                            SetRenderInfo(const SForestRenderInfo& sInfo);
            const SForestRenderInfo&        GetRenderInfo(void) const;
            st_bool                         InitGfx(void);

            // main render functions
            st_bool                         StartRender(void);
            st_bool                         EndRender(void);

            st_bool                         Render3dTrees(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances) const;
            st_bool                         RenderGrass(ERenderPass ePass, const CTreeRI_t* BaseGrass, const CVisibleInstancesRI_t& cVisibleInstances) const;
            st_bool                         RenderBillboards(ERenderPass ePass, const CVisibleInstancesRI_t& sVisibleTrees) const;

            // wind
    virtual void                            WindAdvance(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs);

            // constant buffers
            SFrameConstBuf_t&               GetFrameConstantBuffer(void);
            SFogAndSkyConstBuf_t&           GetFogAndSkyBufferContents(void) const;
            st_bool                         UpdateFrameConstantBuffer(const CView& cView, st_int32 nWindowWidth, st_int32 nWindowHeight);
            st_bool                         UpdateFogAndSkyConstantBuffer(void);

            // statistics
            CRenderStats&                   GetRenderStats(void); // query statistics for last rendered frame

    protected:
            // rendering
            SForestRenderInfo               m_sRenderInfo;

    mutable CRenderStats                    m_cRenderStats;

    private:
                                            CForestRI(const CForestRI& cRight); // copying CForestRI disabled

            TTextureClass                   m_tFizzleNoise;
            TTextureClass                   m_tPerlinNoiseKernel;
            TTextureClass                   m_tAmbientImageLighting;

            // constant buffers
            SFrameConstBuf_t                m_sFrameConstantBuffer;
            SFogAndSkyConstBuf_t            m_sFogAndSkyConstantBuffer;

            // used to sort the draw call list
            struct SDrawCallData
            {
                st_bool                     operator<(const SDrawCallData& sRight) const;
                st_int64                    GetHashKey(void) const;

                CTreeRI_t*                  m_pBaseTree;
                st_int32                    m_nLod;
                const CRenderStateRI_t*     m_pRenderState;
                const SDrawCall*            m_pDrawCall;
                const TGeometryBufferClass* m_pGeometryBuffer;
                st_int32                    m_nBufferOffset;
                CRenderStats::SObjectStats* m_pStats;
                const typename 
                CVisibleInstancesRI_t::
                SForestInstancingData*      m_pInstancingData;
            };
    mutable CArray<SDrawCallData>           m_aSortedDrawCalls;
    };
    #define CForestRI_t CForestRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TInstancingMgrClass, TShaderTechniqueClass, TShaderConstantBufferClass>

    // include inline functions
    #include "ForestRI_inl.h"
    #include "RenderStateRI_inl.h"
    #include "TreeRI_inl.h"
    #include "MiscRI_inl.h"
    #include "RenderStats_inl.h"
    #include "InstancingManagerRI_inl.h"
    #include "VisibleInstancesRI_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
