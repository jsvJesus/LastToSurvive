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
#include "Core/Core.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/String.h"
#include "Core/Wind.h"
#include "Core/HeapAllocCheck.h"
#include "Core/ScopeTrace.h"


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

    const st_int32 c_nMaxNumShadowMaps = 4;
    const st_int32 c_nInvalidRowColIndex = -999999;
    const st_int32 c_nNumFrustumPoints = 8;


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SHeapReserves

    struct ST_DLL_LINK SHeapReserves
    {
                        SHeapReserves( );

            st_int32    m_nMaxBaseTrees;                        // the max number base trees expected to be used in a given forest (not many more than 20-30 is recommended)

            st_int32    m_nMaxVisibleTreeCells;                 // the max number of tree cells visible at any one time in the camera frustum
            st_int32    m_nMaxVisibleGrassCells;                // the max number of grass cells visible at any one time in the camera frustum
            st_int32    m_nMaxVisibleTerrainCells;              // the max number of terrain cells visible at any one time in the camera frustum

            st_int32    m_nMaxTreeInstancesInAnyCell;           // across all base trees, the max number of instances in any tree cell
            st_int32    m_nMaxPerBaseGrassInstancesInAnyCell;   // for any single base grass, the max number of instances in any grass cell

            st_int32    m_nNumShadowMaps;                       // # of expected shadow maps; isn't dynamic and should never exceed 4
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SInstance

    struct ST_DLL_LINK SInstance
    {
                                        SInstance( );

            // matches float4 ATTRO in instance vertex decl
            Vec3                        m_vPos;             
            st_float32                  m_fScalar;          // 1.0 = no scale

            // matches float4 ATTR1 in instance vertex decl
            Vec3                        m_vUp;              // orientation vector
            st_float32                  m_fLodTransition;   // [0.0 - 1.0] indicates amount between discrete LOD levels

            // matches float4 ATTR2 in instance vertex decl
            Vec3                        m_vRight;           // orientation vector
            st_float32                  m_fLodValue;        // [0.0 - 1.0] indicates overall LOD, 0.0 = billboard, 1.0 = highest 3D detail
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure STreeInstance

    struct ST_DLL_LINK STreeInstance : public SInstance
    {
                                        STreeInstance( );

            const CTree*                m_pInstanceOf;          // which base tree this instance represents
            Vec3                        m_vGeometricCenter;     // includes position offset
            st_float32                  m_fCullingRadius;       // radius from instance position to containing sphere
            void*                       m_pUserData;            // generic user placeholder

            // culling utility
            void                        ComputeCullParameters(void);
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SRowCol

    struct SRowCol
    {
                                        SRowCol(st_int32 nRow, st_int32 nCol) :
                                            m_nRow(nRow),
                                            m_nCol(nCol)
                                        {
                                        }

            st_bool                     operator<(const SRowCol& sRight) const { return (m_nRow == sRight.m_nRow) ? (m_nCol < sRight.m_nCol) : (m_nRow < sRight.m_nRow); }
            st_bool                     operator!=(const SRowCol& sRight) const { return (m_nRow != sRight.m_nRow || m_nCol != sRight.m_nCol); }

            st_int32                    m_nRow;
            st_int32                    m_nCol;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Type definitions

    typedef CArray<STreeInstance>           TTreeInstArray;
    typedef CArray<const STreeInstance*>    TTreeInstConstPtrArray;
    typedef CArray<SInstance>               TGrassInstArray;


    ///////////////////////////////////////////////////////////////////////  
    //  Enumeration ECullStatus

    enum ECullStatus
    {
        CS_FULLY_INSIDE_FRUSTUM,        // object is fully inside the frustum
        CS_INTERSECTS_FRUSTUM,          // object is partly in the frustum and partly out
        CS_FULLY_OUTSIDE_FRUSTUM        // object is fully outside of the frustum
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CCell

    class CCell
    {
    public:
            friend class CVisibleInstances;
            template<typename T> friend class CCellHeapMgr;

                                            CCell( );
                                            ~CCell( );

            // cell's grid placement
            void                            SetRowCol(st_int32 nRow, st_int32 nCol);
            st_int32                        Row(void) const;
            st_int32                        Col(void) const;
            st_int32                        UniqueRandomSeed(void) const;
            void                            SetExtents(const CExtents& cExtents);
            const CExtents&                 GetExtents(void) const;

            // culling & LOD values
            ECullStatus                     GetCullStatus(void) const;
            st_float32                      GetLodDistanceSquared(void) const;
            st_float32                      GetLongestBaseTreeLodDistanceSquared(void) const;

            // adds a series of instances to this cell; base trees are passed as a series of CTree 
            // pointers; instances are passed an as array of CInstance pointers; instances should
            // be sorted by base tree affiliation and should be in the same order as the pBaseTrees
            void                            SetTreeInstances(const CTree** pBaseTrees, st_int32 nNumBaseTrees, const STreeInstance** pInstances, st_int32 nNumInstances);

            // returns an array of pointers to tree instances in this cell; the instances may
            // be of more than one base tree; they'll be sorted by base tree affiliation and the
            // base tree order will be what was passed to SetTreeInstances()
           const TTreeInstConstPtrArray&    GetTreeInstances(void) const;

            // grass instance management
            const TGrassInstArray&          GetGrassInstances(void) const;
            void                            SetGrassInstances(const SInstance* pInstances, st_int32 nNumInstances);

            // operator overloads
            st_bool                         operator<(const CCell& cRight) const;
            st_bool                         operator!=(const CCell& cRight) const;

            CMap<const CTree*, CArray<SInstance> >  m_mBaseTreesToBillboardVboStreamsMap; // [base tree #][bb instance #]

    private:
            // grid placement
            st_int32                        m_nRow;
            st_int32                        m_nCol;
            CExtents                        m_cExtents;

            // culling
            st_int32                        m_nFrameIndex;
            ECullStatus                     m_eCullStatus;

            // LOD
            st_float32                      m_fLodDistanceSquared;
            st_float32                      m_fLongestBaseTreeLodDistanceSquared;

            // instances in cell
            TTreeInstConstPtrArray          m_aTreeInstances;   // stores pointers to tree instances
            TGrassInstArray                 m_aGrassInstances;  // stores grass instances
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure S3dTreeInstanceLod

    struct ST_DLL_LINK S3dTreeInstanceLod
    {
            const STreeInstance*        m_pInstance;                    // the instance this object is storing LOD info for
            st_float32                  m_fDistanceFromCameraSquared;   // distance from camera (or LOD ref point)
            st_float32                  m_fLod;                         // [-1.0 to 1.0] value indicating LOD state
            st_int32                    m_nLodLevel;                    // which discrete LOD level is active
            st_float32                  m_fLodTransition;               // LOD hints to the shader system for smooth transitions
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Structure SDetailedCullData

    struct ST_DLL_LINK SDetailedCullData
    {
                                            SDetailedCullData( );

            const CTree*                    m_pBaseTree;
            st_float32                      m_fClosest3dTreeDistanceSquared;        // FLT_MAX if none appear
            st_float32                      m_fClosestBillboardCellDistanceSquared; // FLT_MAX if none appear
            CArray<S3dTreeInstanceLod>      m_a3dInstanceLods;
            CArray<SInstance*>              m_aGpuInstanceBufferPtrs;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Enumerations EPopulationType

    enum EPopulationType
    {
        POPULATION_TREES,
        POPULATION_GRASS
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Type definitions, cont

    typedef CArray<CTree*>                      TTreePtrArray;
    typedef CArray<const CTree*>                TTreeConstPtrArray;
    typedef CArray<S3dTreeInstanceLod>          T3dTreeInstanceLodArray;
    typedef CArray<CCell>                       TCellArray;
    typedef CArray<CCell*>                      TCellPtrArray;
    typedef CMap<SRowCol, CCell*>               TRowColCellPtrMap;
    typedef CArray<TTreeInstArray>              TTreeInstArray2D;
    typedef CArray<SDetailedCullData>           TDetailedCullDataArray;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CCellHeapMgr
    //
    //  Internal use

    template <typename T>
    class ST_DLL_LINK CCellHeapMgr
    {
    public:
                                    CCellHeapMgr(EPopulationType ePopulationType);
                                    ~CCellHeapMgr( );

            void                    Init(st_int32 nMaxNumCells, st_int32 nMaxNumInstancesPerCell);

            T*                      CheckOut(void);
            void                    CheckIn(T* pCell);

    private:
            void                    Grow(void);
            void                    InitCell(T& tCell) const;
            st_bool                 IsInitialized(void) const;

            EPopulationType         m_ePopulationType;

            struct ST_DLL_LINK SHeapBlock
            {
                                    SHeapBlock( )
                                    {
                                        m_aGrassInstBuffer.SetHeapDescription("CCellHeapMgr::m_aGrassInstBuffer");
                                        m_aCellBuffer.SetHeapDescription("CCellHeapMgr::m_aCellBuffer");
                                    }

                TGrassInstArray     m_aGrassInstBuffer;
                TCellArray          m_aCellBuffer;
            };

            CArray<SHeapBlock*>     m_aHeapBlocks;
            CArray<T*>              m_aAvailableCells;

            st_int32                m_nMaxNumCells;
            st_int32                m_nMaxNumInstancesPerCell;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Enumeration EFrustumPlanes

    enum EFrustumPlanes
    {
        ST_PLANE_FIRST,

        ST_PLANE_RIGHT = ST_PLANE_FIRST, ST_PLANE_LEFT, ST_PLANE_BOTTOM, ST_PLANE_TOP, ST_PLANE_NEAR, ST_PLANE_FAR, 

        ST_PLANE_NUM_PLANES
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CView

    class ST_DLL_LINK CView
    {
    public:

                                        CView( );

            // returns true if the values passed in our different from the internal values, false otherwise
            st_bool                     Set(const Vec3& vCameraPos,
                                            const Mat4x4& mProjection,
                                            const Mat4x4& mModelview,
                                            st_float32 fNearClip,
                                            st_float32 fFarClip,
                                            st_bool bPreventLeafCardFlip = false);
            void                        SetLodRefPoint(const Vec3& vLodRefPoint);

            // get parameters set directly
            const Vec3&                 GetCameraPos(void) const;
            const Vec3&                 GetLodRefPoint(void) const;
            const Mat4x4&               GetProjection(void) const;
            const Mat4x4&               GetModelview(void) const;
            const Mat4x4&               GetModelviewNoTranslate(void) const;
            st_float32                  GetNearClip(void) const;
            st_float32                  GetFarClip(void) const;

            // get derived parameters
            const Vec3&                 GetCameraDir(void) const;
            const Mat4x4&               GetComposite(void) const;
            const Mat4x4&               GetCompositeNoTranslate(void) const;
            const Mat4x4&               GetProjectionInverse(void) const;
            st_float32                  GetCameraAzimuth(void) const;
            st_float32                  GetCameraPitch(void) const;
            const Vec3*                 GetFrustumPoints(void) const;
            const Vec4*                 GetFrustumPlanes(void) const;
            const CExtents&             GetFrustumExtents(void) const;

            // get derived-by-request parameters
            const Mat4x4&               GetCameraFacingMatrix(void) const;

            // horizontal billboard support
            void                        SetHorzBillboardFadeAngles(st_float32 fStart, st_float32 fEnd); // in radians
            void                        GetHorzBillboardFadeAngles(st_float32& fStart, st_float32& fEnd) const; // in radians
            st_float32                  GetHorzBillboardFadeValue(void) const; // 0.0 = horz bbs are transparent, 1.0 = horz bb's opaque

    private:
            void                        ComputeCameraFacingMatrix(st_bool bPreventLeafCardFlip);
            void                        ComputeFrustumValues(void);
            void                        ExtractFrustumPlanes(void);

            // parameters are set directly
            Vec3                        m_vCameraPos;
            Vec3                        m_vLodRefPoint;
            Mat4x4                      m_mProjection;
            Mat4x4                      m_mProjectionInverse;
            Mat4x4                      m_mModelview;
            st_float32                  m_fNearClip;
            st_float32                  m_fFarClip;

            // derived
            Vec3                        m_vCameraDir;
            Mat4x4                      m_mComposite;
            Mat4x4                      m_mModelviewNoTranslate;
            Mat4x4                      m_mCompositeNoTranslate;
            st_float32                  m_fCameraAzimuth;
            st_float32                  m_fCameraPitch;
            Vec3                        m_avFrustumPoints[c_nNumFrustumPoints];
            Vec4                        m_avFrustumPlanes[ST_PLANE_NUM_PLANES];
            CExtents                    m_cFrustumExtents;

            // derived on request
            Mat4x4                      m_mCameraFacingMatrix;

            // horizontal billboards
            st_float32                  m_fHorzFadeStartAngle;
            st_float32                  m_fHorzFadeEndAngle;
            st_float32                  m_fHorzFadeValue;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CVisibleInstances

    class ST_DLL_LINK CVisibleInstances
    {
    public:
            friend class CCullingEngine;

                                            CVisibleInstances(EPopulationType ePopulationType = POPULATION_TREES, st_bool bTrackNearestInsts = false);
    virtual                                 ~CVisibleInstances( );

            // init
    virtual void                            SetHeapReserves(const SHeapReserves& sHeapReserves);
            void                            SetCellSize(st_float32 fCellSize); // should be invoked during init and not invoked again
            void                            Clear(void);

            st_bool                         RoughCullCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang);
            st_bool                         FineCullTreeCells(const CView& cView, st_int32 nFrameIndex);
            st_bool                         FineCullGrassCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang);

    virtual void                            Cull3dTreesAndComputeLod(const CView& cView);

            // cell queries
            TCellArray&                     RoughCells(void);
            const TRowColCellPtrMap&        VisibleCells(void) const;
            TCellPtrArray&                  NewlyVisibleCells(void);
            
            // other
            void                            GetExtentsAsRowCols(st_int32& nStartRow, st_int32& nStartCol, st_int32& nEndRow, st_int32& nEndCol) const;
    virtual void                            NotifyOfPopulationChange(void);
            const TDetailedCullDataArray&   Get3dInstanceLods(void) const;

    protected:
            SDetailedCullData*              GetInstaceLodArrayByBase(const CTree* pBaseTree);

            // CVisibleInstances usage hints
            EPopulationType                 m_ePopulationType;      // does this object hold trees or grass
            st_bool                         m_bTrackNearestInsts;   // track nearest 3d inst and bb cell for each instance (slower)

            // cell storage/management
            CCellHeapMgr<CCell>             m_cCellHeapMgr;
            TCellArray                      m_aRoughCullCells;
            TRowColCellPtrMap               m_mVisibleCells;
            TCellPtrArray                   m_aNewlyVisibleCells;

            // instances
            TDetailedCullDataArray          m_aPerBase3dInstances;

            // limit data
            st_float32                      m_fCellSize;

            // optimization, prevent redundant computation
            st_int32                        m_anLastFrustumPointCells[c_nNumFrustumPoints][2];
            st_int32                        m_nFrustumExtentsStartRow;
            st_int32                        m_nFrustumExtentsStartCol;
            st_int32                        m_nFrustumExtentsEndRow;
            st_int32                        m_nFrustumExtentsEndCol;
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Class CForest

    class ST_DLL_LINK CForest
    {
    public:
                                        CForest( );
    virtual                             ~CForest( );

    public:

            // collision
            st_bool                     CollisionAdjust(Vec3& vPoint, const CVisibleInstances& cVisibleInstances, st_int32 nMaxNumTestTrees) const;

            // wind
            void                        WindEnable(st_bool bEnabled);
            st_bool                     WindIsEnabled(void) const;
            void                        WindEnableGusting(const TTreePtrArray& aBaseTrees, st_bool bEnabled);
            st_bool                     WindIsGustingEnabled(void) const;
    virtual void                        WindAdvance(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs);
            void                        WindPreroll(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs);
            void                        WindSetStrength(const TTreePtrArray& aBaseTrees, st_float32 fTreeStrength);
            void                        WindSetInitDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir);
            void                        WindSetDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir);

            st_float32                  WindGetGlobalTime(void) const;

            // render support
            void                        FrameEnd(void);
            st_int32                    GetFrameIndex(void) const;

            // shadows & lighting support
            void                        SetLightDir(const Vec3& vLightDir);
            const Vec3&                 GetLightDir(void) const;
            st_bool                     LightDirChanged(void) const;

    static  st_bool        ST_CALL_CONV ComputeLightView(const Vec3& vLightDir, 
                                                         const Vec3 avMainFrustum[c_nNumFrustumPoints], 
                                                         st_float32 fMapStartDistance,
                                                         st_float32 fMapEndDistance,
                                                         CView& sLightView,
                                                         st_float32 fRearExtension);
            void                        SetCascadedShadowMapDistances(const st_float32 afSplits[c_nMaxNumShadowMaps], st_float32 fFarClip); // each entry marks the end distance of its respective map
            const st_float32*           GetCascadedShadowMapDistances(void) const;
            void                        SetShadowFadePercentage(st_float32 fFade);
            st_float32                  GetShadowFadePercentage(void) const;

    protected:
            // instance storage & culling
            st_int32                    m_nFrameIndex;

            // wind
            st_bool                     m_bWindEnabled;
            st_bool                     m_bWindGustingEnabled;
            st_float32                  m_fWindTime;
            Vec3                        m_vWindDir;

            // shadows & lighting support
            Vec3                        m_vLightDir;
            st_bool                     m_bLightDirChanged;
            st_float32                  m_afCascadedShadowMapSplits[c_nMaxNumShadowMaps + 1];
            st_float32                  m_fShadowFadePercentage;
    };


    // include inline functions
    #include "Forest_inl.h"
    #include "Culling_inl.h"
    #include "Instance_inl.h"
    #include "CellHeapMgr_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
