///////////////////////////////////////////////////////////////////////
//
//  All source files prefixed with "My" indicate an example implementation, 
//  meant to detail what an application might (and, in most cases, should)
//  do when interfacing with the SpeedTree SDK.  These files constitute 
//  the bulk of the SpeedTree Reference Application and are not part
//  of the official SDK, but merely example or "reference" implementations.
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
//  Preprocessor

#pragma once
#include "MySpeedTreeRenderer.h"
#include "MyRenderTargets.h"
#include "MyShadowSystem.h"
#include "MyGrass.h"
#include "MyTerrain.h"
#include "MySky.h"
#include "MyOverlays.h"
#include "MyCmdLineOptions.h"
#include "MyStatsReporter.h"
#include "MyFullscreenQuad.h"


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    ///////////////////////////////////////////////////////////////////////
    //  Enumeration EBloomDisplayMode

    enum EBloomDisplayMode
    {
        BLOOM_DISPLAY_MODE_FULL,
        BLOOM_DISPLAY_MODE_NONE,
        BLOOM_DISPLAY_MODE_OVERLAY,
        BLOOM_DISPLAY_MODE_COUNT
    };


    ///////////////////////////////////////////////////////////////////////
    //  Class CMyApplication
    //
    //  Note that this class is merely an example of how a forest might be initialized;
    //  none of the methods that we have chosen for tree placement & population are
    //  required by the SpeedTree libraries.  You're free to implement it however you like.

    class CMyApplication
    {
    public:
                                            CMyApplication( );
                                            ~CMyApplication( );

            // main functions; initialization/rendering
            st_bool                         ParseCmdLine(st_int32 argc, char* argv[]);
            st_bool                         Init(void);     // init non-graphics resources
            st_bool                         InitGfx(void);  // init graphics resources
            st_bool                         ReadyToRender(void) const;
            st_bool                         InitForwardSupportGfx(void);
            st_bool                         InitDeferredSupportGfx(void);
            st_bool                         InitPostEffectsGfx(void);
            void                            ReleaseGfxResources(void);
            void                            SetWorldTransform(const Mat4x4& mWorldTransform, const Vec3& vCameraPos);
            void                            Render(void);
            void                            DeferredRender(void);
            void                            DeferredRenderWithDepthPrepass(void);
            void                            ForwardRender(void);
            void                            ForwardRenderWithBloom(void);
            void                            ForwardRenderWithDepthPrepass(void);
            void                            ForwardRenderWithDepthPrepassAndBloom(void);
            void                            ForwardRenderWithManualMsaaResolve(void);
            void                            ForwardRenderWithManualMsaaResolveWithDepthPrepass(void);
            void                            EnableTextureAlphaMode(void);
            void                            SetTextureAlphaScalars(st_float32 fScalar3d, st_float32 fScalarGrass, st_float32 fScalarBillboard);
            void                            Advance(void);  // called once per frame
            void                            Cull(void);     // called once per frame
            void                            HandleCollision(void);
            void                            ReportStats(void);
            void                            ReportResourceUsage(void);

            // queries
            const SMyCmdLineOptions&        GetCmdLineOptions(void) const       { return m_sCmdLine; }  // peek into command-line settings
            const CMyConfigFile&            GetConfig(void) const               { return m_cConfigFile; }
            CForestRender*                  GetForest(void)                     { return &m_cForest; }
            CRenderStats&                   GetRenderStats(void)                { return m_cForest.GetRenderStats( ); }
            st_float32                      GetLastFrameTime(void)              { return m_fFrameTimeInSec; }
            Vec3                            ConstrainCameraPos(const Vec3& vInputCameraPos) const;

            // window interactions
            void                            WindowReshape(st_int32 nWindowWidth, st_int32 nWindowHeight);
    static  void                            PrintKeys(void);
    static  void                            PrintId(void);

            // user-input render adjustment
            void                            UpdateLightDir(const Vec3& vLightDir);
            void                            UpdateWindDir(const Vec3& vWindDir);
            void                            UpdateHandTunedValues(const st_float32 afValues[4]);
            enum EToggleFeature
            {
                TOGGLE_3D_TREE_VISIBILITY, TOGGLE_BILLBOARD_VISIBILITY, TOGGLE_BLOOM_DISPLAY_MODE, TOGGLE_COLLISION, 
                TOGGLE_DEPTH_BLUR, TOGGLE_GRASS_VISIBILITY, TOGGLE_OVERLAY_VISIBILITY, TOGGLE_SKY_VISIBILITY, 
                TOGGLE_TERRAIN_FOLLOWING, TOGGLE_TERRAIN_VISIBILITY, TOGGLE_TEXTURING
            };
            void                            ToggleFeature(EToggleFeature eFeature);

            // DX9 support
            void                            OnResetDevice(void);
            void                            OnLostDevice(void);

    private:
            // forest population utilities; InitGfx() will call PopulateForest()
            // which will call the other population functions based on the population
            // method is set on command-line
            st_bool                         PopulateForest(void);
            void                            SetCullCellSizes(void);
            void                            SetHeapReserves(void);
            st_bool                         SetUpGrassLayers(void);
            void                            StreamGrassPopulation(void);
            void                            NotifyOfPopulationChange(void);

            // render support
            void                            StartBloomPostPass(void);
            void                            EndBloomPostPass(void);
            void                            UpdateTimeOfDay(void);

            // diagnostic/stressing functions
            void                            AdjustPopulation(void);

            // main objects
            CForestRender                   m_cForest;                  // main forest object; type is defined in PlatformSpecifics.h
            CMyShadowSystem                 m_cShadowSystem;
            CMyTerrain                      m_cTerrain;
            STerrainCullResults             m_sTerrainCullResults;
            CMySky                          m_cSky;
            CMyOverlays                     m_cOverlays;
            SMyCmdLineOptions               m_sCmdLine;                 // command-line options store here
            CMyConfigFile                   m_cConfigFile;
            CMyStatsReporter                m_cStatsReporter;           // used to report rendering stats to the console

            // deferred rendering
            CMyDeferredRenderTargets        m_cDeferredTarget;
            CRenderState                    m_cDeferredState;
            CMyFullscreenQuad               m_cDeferredRenderResolveQuad;

            // optional bloom post fx
            CMyForwardTargets               m_cBloomForwardTarget;
            CRenderTarget                   m_cBloomAuxTargetA;
            CRenderTarget                   m_cBloomAuxTargetB;
            CRenderState                    m_cBloomHiPassState;
            CMyFullscreenQuad               m_cBloomHiPassQuad;
            CRenderState                    m_cBloomBlurState;
            CMyFullscreenQuad               m_cBloomBlurQuad;
            CRenderState                    m_cBloomFinalState;
            CMyFullscreenQuad               m_cBloomFinalQuad;
            CRenderState                    m_cFullscreenAlpaState;
            CMyFullscreenQuad               m_cFullscreenAlpaQuad;
            SBloomConstBufApp               m_sBloomConstantBuffer;

            // optional depth blur post fx
            CRenderTarget                   m_cDepthBlurTarget;
            CRenderState                    m_cDepthBlurState;
            CMyFullscreenQuad               m_cDepthBlurQuad;

            // explicit resolve support (durango)
            CMyForwardTargets               m_cExplicitMsaaResolveTarget;
            CRenderState                    m_cExplicitMsaaResolveState;
            CMyFullscreenQuad               m_cExplicitMsaaResolveQuad;
            st_bool                         m_bExplicitMsaaResolveEnabled;

            // population
            CMyInstancesContainer           m_cAllTreeInstances;
            SHeapReserves                   m_sHeapReserves;

            // view & culling
            CView                           m_cView;
            st_bool                         m_bCameraChanged;
            CVisibleInstancesRender         m_cVisibleTreesFromCamera;

            // grass
            CArray<CMyGrassLayer>           m_aGrassLayers;
            TGrassInstArray                 m_aRandomGrassInstancesBuffer;

            // keep track of all the base trees & grass models so that their wind
            // can be updated together and can all be deleted on destruction
            TTreePtrArray                   m_aAllBaseTreesAndGrass;
            CArray<CTreeRender*>            m_aAllBaseTrees;
            CArray<CTreeRender*>            m_aAllBaseGrasses;

            // navigation
            st_bool                         m_bFollowTerrain;

            // time management
            st_float32                      m_fCurrentTime;             // time since initialization in seconds
            CTimer                          m_cWallClock;               // real-time wall clock
            st_float32                      m_fFrameTimeInSec;              // time it took to render the last frame in seconds
            st_float32                      m_fTimeMarker;              // used to calculate m_fFrameTime in Advance()
            st_int32                        m_nFrameIndex;

            // state management
            st_bool                         m_bRender3dTrees;           // toggle rendering of 3D tree geometry
            st_bool                         m_bRenderBillboards;        // toggle rendering of billboards
            st_bool                         m_bRenderGrass;
            st_bool                         m_bRenderOverlays;
            ETextureAlphaRenderMode         m_eTransparentTextureRenderMode;
            st_bool                         m_bRenderTransparentMultiPass;
            st_bool                         m_bHandleCollision;
            st_bool                         m_bDepthBlur;
            st_int32                        m_nBloomDisplayMode;

            // window management
            st_bool                         m_bReadyToRender;
            st_float32                      m_fAspectRatio;             // window width / window height
        #ifdef ST_OPENGL_USE_CORE_3_2
            GLuint                          m_uiGeneralVertexArrayObject;
        #endif
    };

} // end namespace SpeedTree

